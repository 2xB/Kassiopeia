#include "KSurfaceContainer.hh"

namespace KEMField
{
KSurfaceContainer::KSurfaceContainer() : fIsOwner(true)
{
    for (auto& i : fPartialSurfaceData)
        for (auto& j : i)
            j = nullptr;

    fSurfaceData = std::make_shared<KSurfaceData>();
}

KSurfaceContainer::~KSurfaceContainer()
{
    clear();
    fSurfaceData.reset();
}

bool operator==(const KSurfaceContainer& lhs, const KSurfaceContainer& rhs)
{
    if (lhs.size() != rhs.size())
        return false;
    // for repeated comparisons of objects of the same (or similar) size, it
    // is faster to reuse a single KDataComparator object
    KDataComparator dC;
    KSurfaceContainer::iterator lit;
    KSurfaceContainer::iterator rit;
    for (lit = lhs.begin(), rit = rhs.begin(); lit != lhs.end(); ++lit, ++rit)
        // if (*(*lit) != *(*rit))
        if (!dC.Compare(*(*lit), *(*rit)))
            return false;
    return true;
}

void KSurfaceContainer::push_back(KSurfacePrimitive* aSurface)
{
    int boundaryPolicy = aSurface->GetID().BoundaryID;
    int shapePolicy = aSurface->GetID().ShapeID;

    auto it = fSurfaceData->begin();
    for (; it != fSurfaceData->end(); ++it)
        if (!(*it)->empty())
            if ((*it)->operator[](0)->GetID().BoundaryID == boundaryPolicy &&
                (*it)->operator[](0)->GetID().ShapeID == shapePolicy) {
                (*it)->push_back(aSurface);
                return;
            }

    fSurfaceData->push_back(new KSurfaceArray(1, aSurface));
}

KSurfacePrimitive* KSurfaceContainer::FirstSurfaceType(unsigned int i) const
{
    if (i >= fSurfaceData->size()) {
        throw KEMSimpleException("KSurfaceContainer::FirstSurfaceType: Could not find element number " + std::to_string(i) + " in surface container of size " + std::to_string(fSurfaceData->size()));
    }

    return fSurfaceData->at(i)->at(0);
}

KSurfacePrimitive* KSurfaceContainer::operator[](const unsigned int& i) const
{
    auto surfaceDataIt = fSurfaceData->begin();
    auto j = i;
    for (; surfaceDataIt != fSurfaceData->end(); ++surfaceDataIt) {
        if (j >= (*surfaceDataIt)->size()) {
            j -= (*surfaceDataIt)->size(); // effectively add next array incides behind previous arrays
            continue;
        }

        KSurfaceArrayCIt surfaceArrayIt = (*surfaceDataIt)->begin();
        std::advance(surfaceArrayIt, j);
        return *surfaceArrayIt;
    }
    throw KEMSimpleException("KSurfaceContainer::operator[] (no policy): Could not find element number " + std::to_string(i) + " in surface container.");
}

unsigned int KSurfaceContainer::size() const
{
    unsigned int i = 0;
    for (auto* it : *fSurfaceData)
        i += it->size();
    return i;
}

KSurfaceContainer::iterator KSurfaceContainer::begin() const
{
    KSurfaceContainer::iterator anIterator;
    anIterator.fData = GetSurfaceData();
    anIterator.fDataIt = anIterator.fData->begin();
    if (!(anIterator.fData->empty()))
        anIterator.fArrayIt = (*anIterator.fDataIt)->begin();

    return anIterator;
}

KSurfaceContainer::iterator KSurfaceContainer::end() const
{
    KSurfaceContainer::iterator anIterator;
    anIterator.fData = GetSurfaceData();
    anIterator.fDataIt = --anIterator.fData->end();
    if (!anIterator.fData->empty())
        anIterator.fArrayIt = (*anIterator.fDataIt)->end();

    return anIterator;
}

void KSurfaceContainer::clear()
{
    for (auto & dataIt : *fSurfaceData) {
        if (fIsOwner)
            for (auto & arrayIt : *dataIt)
                delete arrayIt;
        dataIt->clear();
        delete dataIt;
    }
    fSurfaceData->clear();
}

KSurfaceContainer::SmartDataPointer KSurfaceContainer::GetSurfaceData() const
{
    //return SmartDataPointer(&fSurfaceData, true);
    return fSurfaceData;
}
}  // namespace KEMField
