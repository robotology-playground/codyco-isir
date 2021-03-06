#include "walking-client/constraints/AdmissibilityConstraints.h"

AdmissibilityConstraints::AdmissibilityConstraints() : Constraint() {
    _ssdsAlternation.init();
    _singleSupport.init();
    _contactConfigHistory.init();
    _contactConfigEnforcement.init();
}

AdmissibilityConstraints::~AdmissibilityConstraints() {}

void AdmissibilityConstraints::buildMatrixCi() {
    unsigned int totalRows = _ssdsAlternation.getCi().rows() + _singleSupport.getCi().rows() + _contactConfigHistory.getCi().rows() + _contactConfigEnforcement.getCi().rows();
    unsigned int totalCols = STATE_VECTOR_SIZE;
    OCRA_INFO("Rows: " << totalRows << " Cols: " << totalCols);
    _Ci.resize(totalRows,totalCols);
    _Ci << _ssdsAlternation.getCi(), _singleSupport.getCi(), _contactConfigHistory.getCi(), _contactConfigEnforcement.getCi();
    OCRA_INFO("Built Ci for AdmissibilityConstraints");
}

void AdmissibilityConstraints::buildMatrixCii() {
    unsigned int totalRows = _ssdsAlternation.getCii().rows() + _singleSupport.getCii().rows() + _contactConfigHistory.getCii().rows() + _contactConfigEnforcement.getCii().rows();;
    unsigned int totalCols = STATE_VECTOR_SIZE;
    _Cii.resize(totalRows,totalCols);
    _Cii << _ssdsAlternation.getCii(), _singleSupport.getCii(), _contactConfigHistory.getCii(), _contactConfigEnforcement.getCii();
    OCRA_INFO("Built Cii for AdmissibilityConstraints");
}

void AdmissibilityConstraints::buildVectord() {
    unsigned int totalRows = _ssdsAlternation.getd().size() + _singleSupport.getd().size() + _contactConfigHistory.getd().size() + _contactConfigEnforcement.getd().size();
    _d.resize(totalRows);
    _d << _ssdsAlternation.getd(), _singleSupport.getd(), _contactConfigHistory.getd(), _contactConfigEnforcement.getd();
    OCRA_INFO("Built d for AdmissibilityConstraints");
}
