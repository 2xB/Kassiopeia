# Global args
ARG KASSIOPEIA_UID="1000"
ARG KASSIOPEIA_USER="parrot"
ARG KASSIOPEIA_GID="1000"
ARG KASSIOPEIA_GROUP="kassiopeia"

ARG KASSIOPEIA_GIT_BRANCH=""
ARG KASSIOPEIA_GIT_COMMIT=""

ARG KASSIOPEIA_CPUS=""

# Mesa git ref (tag, branch or commit) to build from source, see the `mesa` stage
ARG MESA_REF="mesa-26.1.6"
ARG MESA_GALLIUM_DRIVERS="llvmpipe,softpipe"

# --- mesa ---
# Fedora 43 ships Mesa 25.3.6, which contains a regression that breaks VTK's
# per-cell scalar coloring: every Kassiopeia geometry render comes out as a
# flat, unlit, uncolored silhouette (Kassiopeia issue #146). Reproducer, version
# matrix and screenshots: Docker/vtk-mesa-fedora43-bug/README.md, upstream
# report: https://gitlab.freedesktop.org/mesa/mesa/-/work_items/15660
#
# The fix is not available as a Fedora 43 package, so Mesa is built from source
# here and installed into /opt/mesa; `runtime-base` then puts it ahead of the
# system Mesa for all following stages.
#
# MESA_REF defaults to mesa-26.1.6, the first upstream release whose release
# notes list the fix. The Fedora 42 version that was known good, mesa-25.1.9,
# can be used instead, but is not guaranteed to build against Fedora 43's LLVM:
#   docker build --build-arg MESA_REF=mesa-25.1.9 ...
#
# Only the software rasterizers are built by default, as the container renders
# through Xvfb/VNC (llvmpipe). Hardware drivers can be added on demand:
#   docker build --build-arg MESA_GALLIUM_DRIVERS=llvmpipe,softpipe,radeonsi,iris ...
FROM fedora:43 as mesa
ARG MESA_REF
ARG MESA_GALLIUM_DRIVERS
ARG KASSIOPEIA_CPUS

LABEL description="Mesa build container"

COPY Docker/packages.mesa packages
RUN dnf update -y \
 && dnf install -y --setopt=install_weak_deps=False $(cat packages) \
 && rm /packages \
 && dnf clean all

WORKDIR /tmp/mesa
RUN git init . \
 && git remote add origin https://gitlab.freedesktop.org/mesa/mesa.git \
 && git fetch --depth 1 origin "$MESA_REF" \
 && git checkout FETCH_HEAD \
 && git log -1 --format='Building Mesa %H (%ai) %s'

RUN meson setup build \
      --prefix=/opt/mesa \
      --libdir=lib64 \
      -Dbuildtype=release \
      -Dstrip=true \
      -Dgallium-drivers=$MESA_GALLIUM_DRIVERS \
      -Dvulkan-drivers=[] \
      -Dplatforms=x11 \
      -Dglx=dri \
      -Dgles1=disabled \
      -Dgles2=enabled \
      -Dopengl=true \
      -Dllvm=enabled \
      -Dvalgrind=disabled \
      -Dlibunwind=disabled \
 && ninja -C build -j${KASSIOPEIA_CPUS:-$(nproc)} \
 && ninja -C build install \
 && cd / && rm -rf /tmp/mesa
# ---

# --- runtime-base ---
FROM fedora:43 as runtime-base
ARG KASSIOPEIA_UID
ARG KASSIOPEIA_USER
ARG KASSIOPEIA_GID
ARG KASSIOPEIA_GROUP

LABEL description="Runtime base container"

COPY Docker/packages.runtime packages
RUN dnf update -y \
 && dnf install -y --setopt=install_weak_deps=False $(cat packages) \
 && rm /packages \
 && dnf clean all

# Use the Mesa built in the `mesa` stage instead of Fedora 43's, which renders
# VTK geometry incorrectly (see the `mesa` stage above). The system Mesa packages
# stay installed but are shadowed by these two variables, so a build without this
# override can be obtained by unsetting them.
COPY --from=mesa /opt/mesa /opt/mesa
ENV LIBGL_DRIVERS_PATH=/opt/mesa/lib64/dri \
    LD_LIBRARY_PATH=/opt/mesa/lib64

# Setting user
# Compare:
# * https://github.com/jupyter/docker-stacks/blob/master/base-notebook/Dockerfile
# * https://docs.docker.com/develop/develop-images/dockerfile_best-practices/

RUN groupadd -g $KASSIOPEIA_GID $KASSIOPEIA_GROUP && useradd --no-log-init -r --create-home -g $KASSIOPEIA_GROUP -u $KASSIOPEIA_UID $KASSIOPEIA_USER \
 && mkdir /kassiopeia \
 && chown $KASSIOPEIA_USER:$KASSIOPEIA_GROUP /kassiopeia

# For backwards compatibility:
RUN ln -s /kassiopeia /home/$KASSIOPEIA_USER/kassiopeia

# Local directory for Python packages
# /kassiopeia/install is created by setup.sh before Python packages are installed
ENV PYTHONUSERBASE=/kassiopeia/install/python
# ---

# --- build-base ---
FROM runtime-base as build-base

LABEL description="Build base container"

COPY Docker/packages.build packages
RUN dnf update -y \
 && dnf install -y --setopt=install_weak_deps=False $(cat packages) \
 && rm /packages \
 && dnf clean all
# ---

# --- build ---
FROM build-base as build
ARG KASSIOPEIA_USER
ARG KASSIOPEIA_GROUP

ARG KASSIOPEIA_GIT_BRANCH
ARG KASSIOPEIA_GIT_COMMIT

ARG KASSIOPEIA_CPUS

LABEL description="Build container"

USER $KASSIOPEIA_USER

COPY --chown=$KASSIOPEIA_USER:$KASSIOPEIA_GROUP . /kassiopeia/code
RUN KASSIOPEIA_BUILD_TYPE="RelWithDebInfo" \
        KASSIOPEIA_INSTALL_PREFIX="/kassiopeia/install" \
        KASSIOPEIA_BUILD_PREFIX="/kassiopeia/build" \
        KASSIOPEIA_MAKECMD="ninja" \
        KASSIOPEIA_CUSTOM_CMAKE_ARGS="-GNinja" \
        /kassiopeia/code/setup.sh && \
    mkdir /kassiopeia/install/log/build && \
    cp /kassiopeia/build/.ninja_log /kassiopeia/install/log/build/ && \
    rm -r /kassiopeia/build && \
    rm -r /kassiopeia/code

COPY --chown=$KASSIOPEIA_USER:$KASSIOPEIA_GROUP Docker/entrypoint.sh /kassiopeia/

# Update /kassiopeia permissions to everyone
USER root
RUN chmod --recursive a=u /kassiopeia
USER $KASSIOPEIA_USER

WORKDIR /kassiopeia

ENTRYPOINT ["/kassiopeia/entrypoint.sh"]
# ---

# --- minimal ---
FROM runtime-base as minimal
ARG KASSIOPEIA_USER

LABEL description="Minimal run container"

USER root

RUN mkdir /kassiopeia/install \
 && chown $KASSIOPEIA_USER:$KASSIOPEIA_GROUP /kassiopeia/install

COPY --from=build /kassiopeia/entrypoint.sh /kassiopeia/entrypoint.sh
COPY --from=build /kassiopeia/install /kassiopeia/install
 
RUN echo /kassiopeia/install/lib64 > /etc/ld.so.conf.d/local-x86_64.conf \
 && ldconfig

# Update /kassiopeia permissions to everyone
RUN chmod --recursive a=u /kassiopeia

USER $KASSIOPEIA_USER

WORKDIR /home/$KASSIOPEIA_USER

ENTRYPOINT ["/kassiopeia/entrypoint.sh"]

CMD ["bash"]
# ---

# --- full-base ---
FROM build-base as full-base

LABEL description="Full base container"

ENV JUPYTER_CONFIG_DIR=/kassiopeia/install/python/.jupyter

COPY Docker/packages.full packages
RUN dnf update -y \
 && dnf install -y --setopt=install_weak_deps=False $(cat packages) \
 && rm /packages \
 && dnf clean all
RUN pip3 install --no-cache-dir jupyterlab \
 && pip3 install --no-cache-dir jupyter-server-proxy \
 && pip3 install --no-cache-dir jupyterhub \
 && pip3 install --no-cache-dir ipympl \
 && pip3 install --no-cache-dir uproot \
 && pip3 install --no-cache-dir iminuit

# Ensure if LDAP is used on a JupyterHub, user names are correctly resolved
# Corresponding packages: nslcd, libnss-ldapd
RUN sed -e 's/^passwd:\(.*\)/passwd:\1 ldap/' -e 's/^group:\(.*\)/group:\1 ldap/' -i /etc/nsswitch.conf
# ---

# --- full ---
# Include build files to enable faster development
FROM full-base as full
ARG KASSIOPEIA_USER

LABEL description="Full run container"

USER root

RUN mkdir /kassiopeia/install \
 && chown $KASSIOPEIA_USER:$KASSIOPEIA_GROUP /kassiopeia/install

COPY --from=build /kassiopeia/entrypoint.sh /kassiopeia/entrypoint.sh
COPY --from=build /kassiopeia/install /kassiopeia/install

RUN echo /kassiopeia/install/lib64 > /etc/ld.so.conf.d/local-x86_64.conf \
 && ldconfig \
 && echo "source /usr/share/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh" >> /home/$KASSIOPEIA_USER/.zshrc

# Always show Kasper information when opening a terminal
RUN echo "source /kassiopeia/install/bin/kasperenv.sh" >> /etc/profile

# JupyterLab VNC desktop environment:
#  Adapted from renku-vnc by SwissDataScienceCenter
#    https://github.com/SwissDataScienceCenter/renku-vnc/tree/c458e1cefc017be657f7068605ad72c7ce91d78d/xvnc4
#  License: Apache License 2.0
#    https://github.com/SwissDataScienceCenter/renku-vnc/blob/5304c95e77b1ef3a71f224bc43582c7dd52b5dc8/LICENSE
# Fix vnc.html
# Fix vnc_lite.html
# Resize to browser window
# Make vnc_lite default
RUN sed -i -e "s,'websockify',window.location.pathname.slice(1),g" /usr/share/novnc/app/ui.js \
    && sed -i -e "s,'websockify',window.location.pathname.slice(1),g" /usr/share/novnc/vnc_lite.html \
    && sed -i -e "s/rfb.scaleViewport = readQueryVariable('scale', false);/rfb.scaleViewport = readQueryVariable('scale', false);rfb.resizeSession = true;/g" /usr/share/novnc/vnc_lite.html \
    && sed -i -e 's,<div id="sendCtrlAltDelButton">Send CtrlAltDel</div>,<div id="sendCtrlAltDelButton" hidden>Send CtrlAltDel</div><div onClick="window.location.reload(true);" style="position: fixed;top: 0px;right: 0px;border: 1px outset;padding: 5px 5px 4px 5px;cursor: pointer;">Reload</div>,g' /usr/share/novnc/vnc_lite.html \
    && ln -fs /usr/share/novnc/vnc_lite.html /usr/share/novnc/index.html
COPY --chown=root:root Docker/startvnc /

# MyBinder mounts /etc/jupyter, making configuration from there e.g. containing terminals inaccessible
# Moving /etc/jupyter to a safer place as a workaround
RUN mkdir -p /usr/etc && mv /etc/jupyter /usr/etc

# Hide Jupyter news announcement
# https://jupyterlab.readthedocs.io/en/stable/user/announcements.html
RUN jupyter labextension disable "@jupyterlab/apputils-extension:announcements"

USER $KASSIOPEIA_USER

# Configure VNC desktop
RUN jupyter lab --generate-config \
    && echo "c.ServerProxy.servers = {\
    'vnc': {\
        'command': ['/startvnc', '{port}'],\
        'timeout' : 10,\
        'absolute_url': False,\
        'new_browser_tab': False,\
        'launcher_entry' :  {\
            'enabled': True,\
            'title': 'VNC (Desktop)'\
        }\
    }\
}" >> $JUPYTER_CONFIG_DIR/jupyter_lab_config.py
# Fix DISPLAY so applications can also use it outside the desktop environment
ENV DISPLAY=:20

# Update /kassiopeia permissions to everyone (needed e.g. in some JupyterHub environments)
USER root
RUN chmod --recursive a=u /kassiopeia
USER $KASSIOPEIA_USER

WORKDIR /home/$KASSIOPEIA_USER

ENTRYPOINT ["/kassiopeia/entrypoint.sh"]

CMD ["jupyter", "lab", "--port=44444", "--ip=0.0.0.0", "--ServerApp.custom_display_url='http://localhost:44444/'"]
# ---
