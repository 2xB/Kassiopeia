#ifndef KOPENCLELECTROSTATICRWGBOUNDARYINTEGRATOR_DEF
#define KOPENCLELECTROSTATICRWGBOUNDARYINTEGRATOR_DEF

#include "KElectrostaticRWGBoundaryIntegrator.hh"
#include "KOpenCLBoundaryIntegrator.hh"
#include "KSurfaceVisitors.hh"

namespace KEMField
{
class ElectrostaticOpenCL;

class KOpenCLElectrostaticRWGBoundaryIntegrator :
    public KOpenCLBoundaryIntegrator<KElectrostaticBasis>,
    public KSelectiveVisitor<KShapeVisitor, KTYPELIST_4(KTriangle, KRectangle, KLineSegment, KConicSection)>
{
  public:
    typedef KElectrostaticBasis Basis;
    typedef Basis::ValueType ValueType;
    typedef KBoundaryType<Basis, KDirichletBoundary> DirichletBoundary;
    typedef KBoundaryType<Basis, KNeumannBoundary> NeumannBoundary;

    // for selection of the correct KIntegratingFieldSolver template and possibly elsewhere
    typedef ElectrostaticOpenCL Kind;
    typedef KElectrostaticRWGBoundaryIntegrator IntegratorSingleThread;

    using KSelectiveVisitor<KShapeVisitor, KTYPELIST_4(KTriangle, KRectangle, KLineSegment, KConicSection)>::Visit;

    KOpenCLElectrostaticRWGBoundaryIntegrator(KOpenCLSurfaceContainer& c);
    ~KOpenCLElectrostaticRWGBoundaryIntegrator();

    void Visit(KTriangle& t)
    {
        ComputeBoundaryIntegral(t);
    }
    void Visit(KRectangle& r)
    {
        ComputeBoundaryIntegral(r);
    }
    void Visit(KLineSegment& l)
    {
        ComputeBoundaryIntegral(l);
    }
    void Visit(KConicSection& c)
    {
        ComputeBoundaryIntegral(c);
    }

    ValueType BoundaryIntegral(KSurfacePrimitive* source, KSurfacePrimitive* target, unsigned int);
    ValueType BoundaryValue(KSurfacePrimitive* surface, unsigned int);
    ValueType& BasisValue(KSurfacePrimitive* surface, unsigned int);

    template<class SourceShape> double Potential(const SourceShape*, const KPosition&) const;
    template<class SourceShape> KFieldVector ElectricField(const SourceShape*, const KPosition&) const;
    template<class SourceShape>
    std::pair<KThreeVector, double> ElectricFieldAndPotential(const SourceShape*, const KPosition&) const;

    std::string OpenCLFile() const
    {
        return "kEMField_ElectrostaticRWGBoundaryIntegrals.cl";
    }

  private:
    class BoundaryVisitor :
        public KSelectiveVisitor<KBoundaryVisitor, KTYPELIST_2(KDirichletBoundary, KNeumannBoundary)>
    {
      public:
        using KSelectiveVisitor<KBoundaryVisitor, KTYPELIST_2(KDirichletBoundary, KNeumannBoundary)>::Visit;

        BoundaryVisitor() {}
        virtual ~BoundaryVisitor() {}

        void Visit(KDirichletBoundary&);
        void Visit(KNeumannBoundary&);

        bool IsDirichlet() const
        {
            return fIsDirichlet;
        }
        ValueType Prefactor() const
        {
            return fPrefactor;
        }
        ValueType GetBoundaryValue() const
        {
            return fBoundaryValue;
        }

      protected:
        bool fIsDirichlet;
        ValueType fPrefactor;
        ValueType fBoundaryValue;
    };

    class BasisVisitor : public KSelectiveVisitor<KBasisVisitor, KTYPELIST_1(KElectrostaticBasis)>
    {
      public:
        using KSelectiveVisitor<KBasisVisitor, KTYPELIST_1(KElectrostaticBasis)>::Visit;

        BasisVisitor() : fBasisValue(NULL) {}
        virtual ~BasisVisitor() {}

        void Visit(KElectrostaticBasis&);

        ValueType& GetBasisValue() const
        {
            return *fBasisValue;
        }

      protected:
        ValueType* fBasisValue;
    };

    template<class SourceShape> void ComputeBoundaryIntegral(SourceShape& source);

    BoundaryVisitor fBoundaryVisitor;
    BasisVisitor fBasisVisitor;
    KSurfacePrimitive* fTarget;
    ValueType fValue;

  private:
    void ConstructOpenCLKernels() const;
    void AssignBuffers() const;

    mutable cl::Kernel* fPhiKernel;
    mutable cl::Kernel* fEFieldKernel;
    mutable cl::Kernel* fEFieldAndPhiKernel;

    mutable cl::Buffer* fBufferPhi;
    mutable cl::Buffer* fBufferEField;
    mutable cl::Buffer* fBufferEFieldAndPhi;
};

template<class SourceShape>
double KOpenCLElectrostaticRWGBoundaryIntegrator::Potential(const SourceShape* source, const KPosition& aPosition) const
{
    StreamSourceToBuffer(source);


    CL_TYPE P[3] = {aPosition[0], aPosition[1], aPosition[2]};

    KOpenCLInterface::GetInstance()->GetQueue().enqueueWriteBuffer(*fBufferP, CL_TRUE, 0, 3 * sizeof(CL_TYPE), P);

    cl::NDRange global(1);
    cl::NDRange local(1);

    KOpenCLInterface::GetInstance()->GetQueue().enqueueNDRangeKernel(*fPhiKernel, cl::NullRange, global, local);

    CL_TYPE phi = 0.;

    KOpenCLInterface::GetInstance()->GetQueue().enqueueReadBuffer(*fBufferPhi, CL_TRUE, 0, sizeof(CL_TYPE), &phi);

    return phi;
}

template<class SourceShape>
KThreeVector KOpenCLElectrostaticRWGBoundaryIntegrator::ElectricField(const SourceShape* source,
                                                                      const KPosition& aPosition) const
{
    StreamSourceToBuffer(source);

    CL_TYPE P[3] = {aPosition[0], aPosition[1], aPosition[2]};

    KOpenCLInterface::GetInstance()->GetQueue().enqueueWriteBuffer(*fBufferP, CL_TRUE, 0, 3 * sizeof(CL_TYPE), P);

    cl::NDRange global(1);
    cl::NDRange local(1);

    KOpenCLInterface::GetInstance()->GetQueue().enqueueNDRangeKernel(*fEFieldKernel, cl::NullRange, global, local);

    CL_TYPE4 eField;

    KOpenCLInterface::GetInstance()->GetQueue().enqueueReadBuffer(*fBufferEField,
                                                                  CL_TRUE,
                                                                  0,
                                                                  sizeof(CL_TYPE4),
                                                                  &eField);

    return KFieldVector(eField.s[0], eField.s[1], eField.s[2]);
}

template<class SourceShape>
std::pair<KThreeVector, double>
KOpenCLElectrostaticRWGBoundaryIntegrator::ElectricFieldAndPotential(const SourceShape* source,
                                                                     const KPosition& aPosition) const
{
    StreamSourceToBuffer(source);

    CL_TYPE P[3] = {aPosition[0], aPosition[1], aPosition[2]};

    KOpenCLInterface::GetInstance()->GetQueue().enqueueWriteBuffer(*fBufferP, CL_TRUE, 0, 3 * sizeof(CL_TYPE), P);

    cl::NDRange global(1);
    cl::NDRange local(1);

    KOpenCLInterface::GetInstance()->GetQueue().enqueueNDRangeKernel(*fEFieldAndPhiKernel,
                                                                     cl::NullRange,
                                                                     global,
                                                                     local);

    CL_TYPE4 eFieldAndPhi;

    KOpenCLInterface::GetInstance()->GetQueue().enqueueReadBuffer(*fBufferEFieldAndPhi,
                                                                  CL_TRUE,
                                                                  0,
                                                                  sizeof(CL_TYPE4),
                                                                  &eFieldAndPhi);

    return std::make_pair(KThreeVector(eFieldAndPhi.s[0], eFieldAndPhi.s[1], eFieldAndPhi.s[2]), eFieldAndPhi.s[3]);
}

template<class SourceShape> void KOpenCLElectrostaticRWGBoundaryIntegrator::ComputeBoundaryIntegral(SourceShape& source)
{
    if (fBoundaryVisitor.IsDirichlet()) {
        fValue = this->Potential(&source, fTarget->GetShape()->Centroid());
    }
    else {
        double dist = (source.Centroid() - fTarget->GetShape()->Centroid()).Magnitude();

        if (dist >= 1.e-12) {
            KFieldVector field = this->ElectricField(&source, fTarget->GetShape()->Centroid());
            fValue = field.Dot(fTarget->GetShape()->Normal());
        }
        else {
            // For planar Neumann elements (here: triangles and rectangles) the following formula
            // is valid and incorporates already the electric field 1./(2.*Eps0).
            // In case of conical (axialsymmetric) Neumann elements this formula has to be modified.
            // Ferenc Glueck and Daniel Hilk, March 27th 2018
            fValue = fBoundaryVisitor.Prefactor() / (2. * KEMConstants::Eps0);
        }
    }
}
}  // namespace KEMField

#endif /* KOPENCLELECTROSTATICRWGBOUNDARYINTEGRATOR_DEF */
