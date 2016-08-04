#include "BundleObservation.h"

#include <QDebug>
#include <QString>
#include <QStringList>

#include "BundleImage.h"
#include "BundleObservationSolveSettings.h"
#include "BundleTargetBody.h"
#include "Camera.h"
#include "SpicePosition.h"
#include "SpiceRotation.h"

using namespace boost::numeric::ublas;

namespace Isis {

  /**
   * Constructs a BundleObservation initialized to a default state.
   */
  BundleObservation::BundleObservation() {
    m_serialNumbers.clear();
    m_imageNames.clear();
    m_parameterNamesList.clear();
    m_observationNumber = "";
    m_instrumentId = "";
    m_instrumentRotation = NULL;
    m_instrumentPosition = NULL;
    m_index = 0;
    m_weights.clear();
    m_corrections.clear();
//    m_solution.clear();
    m_aprioriSigmas.clear();
    m_adjustedSigmas.clear();
  }


  /**
   * Constructs a BundleObservation from an BundleImage, an instrument id, an observation
   * number to assign to this BundleObservation, and a target body.
   *
   * @param image QSharedPointer to the primary image in the observation  
   * @param observationNumber Observation number of the observation
   * @param instrumentId Id of the instrument for the observation
   * @param bundleTargetBody QSharedPointer to the target body of the observation
   */
  BundleObservation::BundleObservation(BundleImageQsp image, QString observationNumber,
                                       QString instrumentId, BundleTargetBodyQsp bundleTargetBody) {
    m_serialNumbers.clear();
    m_imageNames.clear();
    m_parameterNamesList.clear();
    m_observationNumber = "";
    m_instrumentId = "";
    m_instrumentRotation = NULL;
    m_instrumentPosition = NULL;
    m_index = 0;
    m_weights.clear();
    m_corrections.clear();
//    m_solution.clear();
    m_aprioriSigmas.clear();
    m_adjustedSigmas.clear();

    m_observationNumber = observationNumber;
    m_instrumentId = instrumentId;

    m_bundleTargetBody = bundleTargetBody;

    if (image) {
      append(image);
      m_serialNumbers.append(image->serialNumber());
      m_imageNames.append(image->fileName());

      // set the observations spice position and rotation objects from the primary image in the
      // observation (this is, by design at the moment, the first image added to the observation)
      // if the image, camera, or instrument position/orientation is null, then set to null
      m_instrumentPosition = (image->camera() ? 
                               (image->camera()->instrumentPosition() ?
                                 image->camera()->instrumentPosition() : NULL)
                               : NULL);
      m_instrumentRotation = (image->camera() ? 
                               (image->camera()->instrumentRotation() ?
                                  image->camera()->instrumentRotation() : NULL)
                               : NULL);

      // set the observations target body spice rotation object from the primary image in the
      // observation (this is, by design at the moment, the first image added to the observation)
      // if the image, camera, or instrument position/orientation is null, then set to null
//      m_bodyRotation = (image->camera() ?
//                           (image->camera()->bodyRotation() ?
//                             image->camera()->bodyRotation() : NULL)
//                           : NULL);
    }
  }


  /**
   * Creates a copy of another BundleObservation.
   *
   * @param src Reference to the BundleObservation to copy
   */
  BundleObservation::BundleObservation(const BundleObservation &src) {
    m_serialNumbers = src.m_serialNumbers;

    m_observationNumber = src.m_observationNumber;
    m_instrumentId = src.m_instrumentId;

    m_instrumentPosition = src.m_instrumentPosition;
    m_instrumentRotation = src.m_instrumentRotation;

    m_solveSettings = src.m_solveSettings;

    m_index = src.m_index;
  }


  /**
   * Destructor.
   *
   * Upon destruction, any contained QObjects (BundleImages) will be deleted.
   */
  BundleObservation::~BundleObservation() {
    clear();
  }


  /**
   * Assignment operator
   *
   * Assigns the state of the source BundleObservation to this BundleObservation
   *
   * @param BundleObservation Reference to the source BundleObservation to assign from
   *
   * @return @b BundleObservation& Reference to this BundleObservation
   */
  BundleObservation &BundleObservation::operator=(const BundleObservation &src) {
    if (&src != this) {
      m_serialNumbers = src.m_serialNumbers;

      m_observationNumber = src.m_observationNumber;
      m_instrumentId = src.m_instrumentId;

      m_instrumentPosition = src.m_instrumentPosition;
      m_instrumentRotation = src.m_instrumentRotation;

      m_solveSettings = src.m_solveSettings;
    }
    return *this;
  }


  /**
   * Set solve parameters
   *
   * @param solveSettings The solve settings to use
   *
   * @return @b bool Returns true if settings were successfully set
   *
   * @internal
   *   @todo initParameterWeights() doesn't return false, so this methods always 
   *         returns true.
   */
  bool BundleObservation::setSolveSettings(BundleObservationSolveSettings solveSettings) {
    m_solveSettings = BundleObservationSolveSettingsQsp(
                        new BundleObservationSolveSettings(solveSettings));

    // initialize solution parameters for this observation
    int nCameraAngleCoefficients = m_solveSettings->numberCameraAngleCoefficientsSolved();
    int nCameraPositionCoefficients = m_solveSettings->numberCameraPositionCoefficientsSolved();

    int nParameters = 3*nCameraPositionCoefficients + 2*nCameraAngleCoefficients;
    if (nCameraAngleCoefficients >= 1 && m_solveSettings->solveTwist()) {
      nParameters += nCameraAngleCoefficients;
    }
    // size vectors and set to zero
    m_weights.resize(nParameters);
    m_weights.clear();
    m_corrections.resize(nParameters);
    m_corrections.clear();
//    m_solution.resize(nParameters);
//    m_solution.clear();
    m_adjustedSigmas.resize(nParameters);
    m_adjustedSigmas.clear();
    m_aprioriSigmas.resize(nParameters);
    for ( int i = 0; i < nParameters; i++) // initialize apriori sigmas to -1.0
      m_aprioriSigmas[i] = Isis::Null;

    if (!initParameterWeights()) {
      // TODO: some message here!!!!!!!!!!!
      // TODO:  do we need this??? initParameterWeights() never returns false...
      return false;
    }

    return true;
  }


  /**
   * Accesses the instrument id
   *
   * @return @b QString Returns the instrument id of the observation
   */
  QString BundleObservation::instrumentId() {
    return m_instrumentId;
  }


  /**
   * Accesses the instrument's spice rotation
   *
   * @return @b SpiceRotation* Returns the SpiceRotation for this observation
   */
  SpiceRotation *BundleObservation::spiceRotation() {
    return m_instrumentRotation;
  }


  /**
   * Accesses the instrument's spice position 
   *
   * @return @b SpicePosition* Returns the SpicePosition for this observation
   */
  SpicePosition *BundleObservation::spicePosition() {
    return m_instrumentPosition;
  }


  /**
   * Accesses the solve parameter weights
   * 
   * @return @b vector<double> Returns the parameter weights for solving
   */
  vector<double> &BundleObservation::parameterWeights() {
    return m_weights;
  }


  /**
   * Accesses the parameter corrections 
   *
   * @return @b vector<double> Returns the parameter corrections
   */
  vector<double> &BundleObservation::parameterCorrections() {
    return m_corrections;
  }


  /**
   * @internal
   *   @todo 
   */
//  vector<double> &BundleObservation::parameterSolution() {
//    return m_solution;
//  }


  /**
   * Accesses the a priori sigmas
   *
   * @return @b vecotr<double> Returns the a priori sigmas
   */
  vector<double> &BundleObservation::aprioriSigmas() {
    return m_aprioriSigmas;
  }


  /**
   * Accesses the adjusted sigmas 
   *
   * @return @b vector<double> Returns the adjusted sigmas
   */
  vector<double> &BundleObservation::adjustedSigmas() {
    return m_adjustedSigmas;
  }


  /**
   * Accesses the solve settings
   *
   * @return @b const BundleObservationSolveSettingsQsp Returns a pointer to the solve
   *                                                    settings for this BundleObservation
   */
  const BundleObservationSolveSettingsQsp BundleObservation::solveSettings() { 
    return m_solveSettings;
  }


  /**
   * Initializes the exterior orientation 
   *
   * @return @b bool Returns true upon successful intialization
   *
   * @internal
   *   @todo Should this always return true?
   */
  bool BundleObservation::initializeExteriorOrientation() {

    if (m_solveSettings->instrumentPositionSolveOption() !=
        BundleObservationSolveSettings::NoPositionFactors) {

      double positionBaseTime = 0.0;
      double positiontimeScale = 0.0;
      std::vector<double> posPoly1, posPoly2, posPoly3;

      for (int i = 0; i < size(); i++) {
        BundleImageQsp image = at(i);
        SpicePosition *spicePosition = image->camera()->instrumentPosition();

        if (i > 0) {
          spicePosition->SetPolynomialDegree(m_solveSettings->spkSolveDegree());
          spicePosition->SetOverrideBaseTime(positionBaseTime, positiontimeScale);
          spicePosition->SetPolynomial(posPoly1, posPoly2, posPoly3,
                                       m_solveSettings->positionInterpolationType());
        }
        else {
          // first, set the degree of the spk polynomial to be fit for a priori values
          spicePosition->SetPolynomialDegree(m_solveSettings->spkDegree());

          // now, set what kind of interpolation to use (SPICE, memcache, hermitecache, polynomial
          // function, or polynomial function over constant hermite spline)
          // TODO: verify - I think this actually performs the a priori fit
          spicePosition->SetPolynomial(m_solveSettings->positionInterpolationType());

          // finally, set the degree of the spk polynomial actually used in the bundle adjustment
          spicePosition->SetPolynomialDegree(m_solveSettings->spkSolveDegree());

          if (m_instrumentPosition) { // ??? TODO: why is this different from rotation code below???
            positionBaseTime = m_instrumentPosition->GetBaseTime();
            positiontimeScale = m_instrumentPosition->GetTimeScale();
            m_instrumentPosition->GetPolynomial(posPoly1, posPoly2, posPoly3);
          }
        }
      }
    }

    if (m_solveSettings->instrumentPointingSolveOption() !=
        BundleObservationSolveSettings::NoPointingFactors) {

      double rotationBaseTime = 0.0;
      double rotationtimeScale = 0.0;
      std::vector<double> anglePoly1, anglePoly2, anglePoly3;

      for (int i = 0; i < size(); i++) {
        BundleImageQsp image = at(i);
        SpiceRotation *spicerotation = image->camera()->instrumentRotation();

        if (i > 0) {
          spicerotation->SetPolynomialDegree(m_solveSettings->ckSolveDegree());
          spicerotation->SetOverrideBaseTime(rotationBaseTime, rotationtimeScale);
          spicerotation->SetPolynomial(anglePoly1, anglePoly2, anglePoly3,
                                       m_solveSettings->pointingInterpolationType());
        }
        else {
          // first, set the degree of the spk polynomial to be fit for a priori values
          spicerotation->SetPolynomialDegree(m_solveSettings->ckDegree());

          // now, set what kind of interpolation to use (SPICE, memcache, hermitecache, polynomial
          // function, or polynomial function over constant hermite spline)
          // TODO: verify - I think this actually performs the a priori fit
          spicerotation->SetPolynomial(m_solveSettings->pointingInterpolationType());

          // finally, set the degree of the spk polynomial actually used in the bundle adjustment
          spicerotation->SetPolynomialDegree(m_solveSettings->ckSolveDegree());

          rotationBaseTime = spicerotation->GetBaseTime();
          rotationtimeScale = spicerotation->GetTimeScale();
          spicerotation->GetPolynomial(anglePoly1, anglePoly2, anglePoly3);
        }
      }
    }

    return true;
  }


  /**
   * Intializes the body rotation 
   *
   * @todo check to make sure m_bundleTargetBody is valid
   */
  void BundleObservation::initializeBodyRotation() {
    std::vector<Angle> raCoefs = m_bundleTargetBody->poleRaCoefs();
    std::vector<Angle> decCoefs = m_bundleTargetBody->poleDecCoefs();
    std::vector<Angle> pmCoefs = m_bundleTargetBody->pmCoefs();

    for (int i = 0; i < size(); i++) {
      BundleImageQsp image = at(i);
      image->camera()->bodyRotation()->setPckPolynomial(raCoefs, decCoefs, pmCoefs);      
    }
  }


  /**
   * Updates the body rotation 
   *
   * @internal
   *   @todo Is this a duplicate of initializeBodyRotation?
   */
  void BundleObservation::updateBodyRotation() {
    std::vector<Angle> raCoefs = m_bundleTargetBody->poleRaCoefs();
    std::vector<Angle> decCoefs = m_bundleTargetBody->poleDecCoefs();
    std::vector<Angle> pmCoefs = m_bundleTargetBody->pmCoefs();

    for (int i = 0; i < size(); i++) {
      BundleImageQsp image = at(i);
      image->camera()->bodyRotation()->setPckPolynomial(raCoefs, decCoefs, pmCoefs);
    }
  }


/*
  bool BundleObservation::initializeExteriorOrientation() {

    if (m_solveSettings->instrumentPositionSolveOption() !=
        BundleObservationSolveSettings::NoPositionFactors) {

      for (int i = 0; i < size(); i++) {
        BundleImageQsp image = at(i);
        SpicePosition *spiceposition = image->camera()->instrumentPosition();

        // first, set the degree of the spk polynomial to be fit for a priori values
        spiceposition->SetPolynomialDegree(m_solveSettings->spkDegree());

        // now, set what kind of interpolation to use (SPICE, memcache, hermitecache, polynomial
        // function, or polynomial function over constant hermite spline)
        // TODO: verify - I think this actually performs the a priori fit
        spiceposition->SetPolynomial(m_solveSettings->positionInterpolationType());

        // finally, set the degree of the spk polynomial actually used in the bundle adjustment
        spiceposition->SetPolynomialDegree(m_solveSettings->spkSolveDegree());
      }
    }

    if (m_solveSettings->instrumentPointingSolveOption() !=
        BundleObservationSolveSettings::NoPointingFactors) {

      for (int i = 0; i < size(); i++) {
        BundleImageQsp image = at(i);
        SpiceRotation *spicerotation = image->camera()->instrumentRotation();

        // first, set the degree of the spk polynomial to be fit for a priori values
        spicerotation->SetPolynomialDegree(m_solveSettings->ckDegree());

        // now, set what kind of interpolation to use (SPICE, memcache, hermitecache, polynomial
        // function, or polynomial function over constant hermite spline)
        // TODO: verify - I think this actually performs the a priori fit
        spicerotation->SetPolynomial(m_solveSettings->pointingInterpolationType());

        // finally, set the degree of the spk polynomial actually used in the bundle adjustment
        spicerotation->SetPolynomialDegree(m_solveSettings->ckSolveDegree());
      }
    }

    return true;
  }
*/


  /**
   * Initializes the paramater weights for solving
   * 
   * @return @b bool Returns true upon successful intialization
   *
   * @internal  
   *   @todo Don't like this, don't like this, don't like this, don't like this, don't like this.
   *         By the way, this seems klunky to me, would like to come up with a better way.
   *         Also, apriori sigmas are in two places, the BundleObservationSolveSettings AND in the
   *         the BundleObservation class too - this is unnecessary should only be in the
   *         BundleObservationSolveSettings. But, they are split into position and pointing.
   *
   *   @todo always returns true?
   */
  bool BundleObservation::initParameterWeights() {

                                   // weights for
    double posWeight    = 0.0;     // position
    double velWeight    = 0.0;     // velocity
    double accWeight    = 0.0;     // acceleration
    double angWeight    = 0.0;     // angles
    double angVelWeight = 0.0;     // angular velocity
    double angAccWeight = 0.0;     // angular acceleration

    QList<double> aprioriPointingSigmas = m_solveSettings->aprioriPointingSigmas();
    QList<double> aprioriPositionSigmas = m_solveSettings->aprioriPositionSigmas();

    int nCamPosCoeffsSolved = 3  *m_solveSettings->numberCameraPositionCoefficientsSolved();

    int nCamAngleCoeffsSolved;
    if (m_solveSettings->solveTwist()) {
      nCamAngleCoeffsSolved = 3  *m_solveSettings->numberCameraAngleCoefficientsSolved();
    }
    else {
      nCamAngleCoeffsSolved = 2  *m_solveSettings->numberCameraAngleCoefficientsSolved();
    }

    if (aprioriPositionSigmas.size() >= 1 && aprioriPositionSigmas.at(0) > 0.0) {
      posWeight = aprioriPositionSigmas.at(0);
      posWeight = 1.0 / (posWeight  *posWeight * 1.0e-6);
    }
    if (aprioriPositionSigmas.size() >= 2 && aprioriPositionSigmas.at(1) > 0.0) {
      velWeight = aprioriPositionSigmas.at(1);
      velWeight = 1.0 / (velWeight  *velWeight * 1.0e-6);
    }
    if (aprioriPositionSigmas.size() >= 3 && aprioriPositionSigmas.at(2) > 0.0) {
      accWeight = aprioriPositionSigmas.at(2);
      accWeight = 1.0 / (accWeight  *accWeight * 1.0e-6);
    }

    if (aprioriPointingSigmas.size() >= 1 && aprioriPointingSigmas.at(0) > 0.0) {
      angWeight = aprioriPointingSigmas.at(0);
      angWeight = 1.0 / (angWeight  *angWeight * DEG2RAD * DEG2RAD);
    }
    if (aprioriPointingSigmas.size() >= 2 && aprioriPointingSigmas.at(1) > 0.0) {
      angVelWeight = aprioriPointingSigmas.at(1);
      angVelWeight = 1.0 / (angVelWeight * angVelWeight * DEG2RAD * DEG2RAD);
    }
    if (aprioriPointingSigmas.size() >= 3 && aprioriPointingSigmas.at(2) > 0.0) {
      angAccWeight = aprioriPointingSigmas.at(2);
      angAccWeight = 1.0 / (angAccWeight * angAccWeight * DEG2RAD * DEG2RAD);
    }

    int nSpkTerms = m_solveSettings->spkSolveDegree()+1;
    nSpkTerms = m_solveSettings->numberCameraPositionCoefficientsSolved();
    for ( int i = 0; i < nCamPosCoeffsSolved; i++) {
      if (i % nSpkTerms == 0) {
       m_aprioriSigmas[i] = aprioriPositionSigmas.at(0);
       m_weights[i] = posWeight;
      }
      if (i % nSpkTerms == 1) {
       m_aprioriSigmas[i] = aprioriPositionSigmas.at(1);
       m_weights[i] = velWeight;
      }
      if (i % nSpkTerms == 2) {
       m_aprioriSigmas[i] = aprioriPositionSigmas.at(2);
       m_weights[i] = accWeight;
      }
    }

    int nCkTerms = m_solveSettings->ckSolveDegree()+1;
    nCkTerms = m_solveSettings->numberCameraAngleCoefficientsSolved();
    for ( int i = 0; i < nCamAngleCoeffsSolved; i++) {
      if (i % nCkTerms == 0) {
        m_aprioriSigmas[nCamPosCoeffsSolved + i] = aprioriPointingSigmas.at(0);
        m_weights[nCamPosCoeffsSolved + i] = angWeight;
      }
      if (i % nCkTerms == 1) {
        m_aprioriSigmas[nCamPosCoeffsSolved + i] = aprioriPointingSigmas.at(1);
        m_weights[nCamPosCoeffsSolved + i] = angVelWeight;
      }
      if (i % nCkTerms == 2) {
        m_aprioriSigmas[nCamPosCoeffsSolved + i] = aprioriPointingSigmas.at(2);
        m_weights[nCamPosCoeffsSolved + i] = angAccWeight;
      }
    }

//    for ( int i = 0; i < (int)m_weights.size(); i++ )
//      std::cout << m_weights[i] << std::endl;

    return true;
  }


  /**
   * Applies the parameter corrections 
   *
   * @param corrections Vector of corrections to apply
   *
   * @throws IException::Unknown "Instrument position is NULL, but position solve option is 
   *                              [not NoPositionFactors]"
   * @throws IException::Unknown "Instrument position is NULL, but pointing solve option is
   *                              [not NoPointingFactors]"
   * @throws IException::Unknown "Unable to apply parameter corrections to BundleObservation."
   *
   * @return @b bool Returns true upon successful application of corrections
   *
   * @internal
   *   @todo always returns true?
   */  
  bool BundleObservation::applyParameterCorrections(
      boost::numeric::ublas::vector<double> corrections) {

    int index = 0;

    try {
      int nCameraAngleCoefficients = m_solveSettings->numberCameraAngleCoefficientsSolved();
      int nCameraPositionCoefficients = m_solveSettings->numberCameraPositionCoefficientsSolved();

      BundleObservationSolveSettings::InstrumentPositionSolveOption positionOption
          = m_solveSettings->instrumentPositionSolveOption();
      if (positionOption != BundleObservationSolveSettings::NoPositionFactors) {

        if (!m_instrumentPosition) {
          QString msg = "Instrument position is NULL, but position solve option is ";
          msg.append(BundleObservationSolveSettings::instrumentPositionSolveOptionToString(
                     positionOption));
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        std::vector<double> coefX(nCameraPositionCoefficients);
        std::vector<double> coefY(nCameraPositionCoefficients);
        std::vector<double> coefZ(nCameraPositionCoefficients);

        m_instrumentPosition->GetPolynomial(coefX, coefY, coefZ);

        // update X coordinate coefficient(s) and sum parameter correction
        for (int i = 0; i < nCameraPositionCoefficients; i++) {
          coefX[i] += corrections(index);
          index++;
        }

        // update Y coordinate coefficient(s) and sum parameter correction
        for (int i = 0; i < nCameraPositionCoefficients; i++) {
          coefY[i] += corrections(index);
          index++;
        }

        // update Z coordinate coefficient(s) and sum parameter correction
        for (int i = 0; i < nCameraPositionCoefficients; i++) {
          coefZ[i] += corrections(index);
          index++;
        }

        // apply updates to all images in observation
        for (int i = 0; i < size(); i++) {
          BundleImageQsp image = at(i);
          SpicePosition *spiceposition = image->camera()->instrumentPosition();
          spiceposition->SetPolynomial(coefX, coefY, coefZ,
                                       m_solveSettings->positionInterpolationType());
        }
      }

      BundleObservationSolveSettings::InstrumentPointingSolveOption pointingOption
          = m_solveSettings->instrumentPointingSolveOption();
      if (pointingOption != BundleObservationSolveSettings::NoPointingFactors) {

        if (!m_instrumentRotation) {
          QString msg = "Instrument rotation is NULL, but pointing solve option is ";
          msg.append(BundleObservationSolveSettings::instrumentPointingSolveOptionToString(
                     pointingOption));
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        std::vector<double> coefRA(nCameraPositionCoefficients);
        std::vector<double> coefDEC(nCameraPositionCoefficients);
        std::vector<double> coefTWI(nCameraPositionCoefficients);

        m_instrumentRotation->GetPolynomial(coefRA, coefDEC, coefTWI);

        // update RA coefficient(s)
        for (int i = 0; i < nCameraAngleCoefficients; i++) {
          coefRA[i] += corrections(index);
          index++;
        }

        // update DEC coefficient(s)
        for (int i = 0; i < nCameraAngleCoefficients; i++) {
          coefDEC[i] += corrections(index);
          index++;
        }

        if (m_solveSettings->solveTwist()) {
          // update TWIST coefficient(s)
          for (int i = 0; i < nCameraAngleCoefficients; i++) {
            coefTWI[i] += corrections(index);
            index++;
          }
        }

        // apply updates to all images in observation
        for (int i = 0; i < size(); i++) {
          BundleImageQsp image = at(i);
          SpiceRotation *spiceRotation = image->camera()->instrumentRotation();
          spiceRotation->SetPolynomial(coefRA, coefDEC, coefTWI,
                                       m_solveSettings->pointingInterpolationType());
        }
      }

      // update corrections
      m_corrections += corrections;

    } 
    catch (IException &e) {
      QString msg = "Unable to apply parameter corrections to BundleObservation.";
      IException(e, IException::Unknown, msg, _FILEINFO_); //THROW ???
    }
    return true;
  }


  /**
   * Returns the number of position parameters there are
   *
   * @return @b int Returns the number of position parameters
   */
  int BundleObservation::numberPositionParameters() {
    return 3.0 * m_solveSettings->numberCameraPositionCoefficientsSolved();
  }


  /**
   * Returns the number of pointing parameters being solved for
   *
   * @return @b int Returns the number of pointing parameters
   */
  int BundleObservation::numberPointingParameters() {
    int angleCoefficients = m_solveSettings->numberCameraAngleCoefficientsSolved();

    if (m_solveSettings->solveTwist()) {
      return 3.0 * angleCoefficients;
    }
    return 2.0 * angleCoefficients;
  }


  /**
   * Returns the number of total parameters there are for solving
   *
   * The total number of parameters is equal to the number of position parameters and number of
   * pointing parameters
   *
   * @return @b int Returns the number of parameters there are
   */
  int BundleObservation::numberParameters() {
    return numberPositionParameters() + numberPointingParameters();
  }


  /**
   * Sets the index for the observation
   *
   * @param n Value to set the index of the observation to
   */
  void BundleObservation::setIndex(int n) {
    m_index = n;
  }


  /**
   * Accesses the observation's index
   *
   * @return @b int Returns the observation's index
   */
  int BundleObservation::index() {
    return m_index;
  }


  /**
   * Creates and returns a formatted QString representing the bundle coefficients and 
   * parameters
   *
   * @param errorPropagation Boolean indicating whether or not to attach more information 
   *     (corrections, sigmas, adjusted sigmas...) to the output QString
   *
   * @return @b QString Returns a formatted QString representing the BundleObservation 
   */
  QString BundleObservation::formatBundleOutputString(bool errorPropagation) {
    std::vector<double> coefX;
    std::vector<double> coefY;
    std::vector<double> coefZ;
    std::vector<double> coefRA;
    std::vector<double> coefDEC;
    std::vector<double> coefTWI;

    int nPositionCoefficients = m_solveSettings->numberCameraPositionCoefficientsSolved();
    int nPointingCoefficients = m_solveSettings->numberCameraAngleCoefficientsSolved();

    int nPositionParameters = numberPositionParameters();
    int nPointingParameters = numberPointingParameters();
    int nParameters = nPositionParameters + nPointingParameters;

    coefX.resize(nPositionCoefficients);
    coefY.resize(nPositionCoefficients);
    coefZ.resize(nPositionCoefficients);
    coefRA.resize(nPointingCoefficients);
    coefDEC.resize(nPointingCoefficients);
    coefTWI.resize(nPointingCoefficients);

    if (nPositionCoefficients > 0 && m_instrumentPosition) {
      m_instrumentPosition->GetPolynomial(coefX, coefY, coefZ);
    }

    if (nPointingCoefficients > 0 && m_instrumentRotation) {
      m_instrumentRotation->GetPolynomial(coefRA, coefDEC, coefTWI);
    }

    // for convenience, create vectors of parameters names and values in the correct sequence
    std::vector<double> finalParameterValues;
    QStringList parameterNamesList;
    QString str("%1(t%2)");
    if (nPositionCoefficients > 0) {
      for (int i = 0; i < nPositionCoefficients; i++) {
        finalParameterValues.push_back(coefX[i]);
        if (i == 0)
          parameterNamesList.append( str.arg("  X  ").arg("0") );
        else
          parameterNamesList.append( str.arg("     ").arg(i) );
      }
      for (int i = 0; i < nPositionCoefficients; i++) {
        finalParameterValues.push_back(coefY[i]);
        if (i == 0)
          parameterNamesList.append( str.arg("  Y  ").arg("0") );
        else
          parameterNamesList.append( str.arg("     ").arg(i) );
      }
      for (int i = 0; i < nPositionCoefficients; i++) {
        finalParameterValues.push_back(coefZ[i]);
        if (i == 0)
          parameterNamesList.append( str.arg("  Z  ").arg("0") );
        else
          parameterNamesList.append( str.arg("     ").arg(i) );
      }
    }
    if (nPointingCoefficients > 0) {
      for (int i = 0; i < nPointingCoefficients; i++) {
        finalParameterValues.push_back(coefRA[i] * RAD2DEG);
        if (i == 0)
          parameterNamesList.append( str.arg(" RA  ").arg("0") );
        else
          parameterNamesList.append( str.arg("     ").arg(i) );
      }
      for (int i = 0; i < nPointingCoefficients; i++) {
        finalParameterValues.push_back(coefDEC[i] * RAD2DEG);
        if (i == 0)
          parameterNamesList.append( str.arg("DEC  ").arg("0") );
        else
          parameterNamesList.append( str.arg("     ").arg(i) );
      }
      for (int i = 0; i < nPointingCoefficients; i++) {
        finalParameterValues.push_back(coefTWI[i] * RAD2DEG);
        if (i == 0)
          parameterNamesList.append( str.arg("TWI  ").arg("0") );
        else
          parameterNamesList.append( str.arg("     ").arg(i) );
      }
    }

    m_parameterNamesList = parameterNamesList;
    QString finalqStr = "";
    QString qStr = "";
    QString sigma = "";

    // position parameters
    for (int i = 0; i < nPositionParameters; i++) {

      sigma = ( IsSpecial(m_aprioriSigmas[i]) ? "N/A" : toString(m_aprioriSigmas[i], 8) );

      if (errorPropagation) {
        qStr = QString("%1%2%3%4%5%6\n").
            arg( parameterNamesList.at(i) ).
            arg(finalParameterValues[i] - m_corrections(i), 17, 'f', 8).
            arg(m_corrections(i), 21, 'f', 8).
            arg(finalParameterValues[i], 20, 'f', 8).
            arg(sigma, 18).
            arg(m_adjustedSigmas[i], 18, 'f', 8);
      }
      else {
        qStr = QString("%1%2%3%4%5%6\n").
            arg( parameterNamesList.at(i) ).
            arg(finalParameterValues[i] - m_corrections(i), 17, 'f', 8).
            arg(m_corrections(i), 21, 'f', 8).
            arg(finalParameterValues[i], 20, 'f', 8).
            arg(sigma, 18).
            arg("N/A", 18);
      }

      finalqStr += qStr;
    }

    // pointing parameters
    for (int i = nPositionParameters; i < nParameters; i++) {

      sigma = ( IsSpecial(m_aprioriSigmas[i]) ? "N/A" : toString(m_aprioriSigmas[i], 8) );

      if (errorPropagation) {
        qStr = QString("%1%2%3%4%5%6\n").
            arg( parameterNamesList.at(i) ).
            arg((finalParameterValues[i] - m_corrections(i) * RAD2DEG), 17, 'f', 8).
            arg(m_corrections(i) * RAD2DEG, 21, 'f', 8).
            arg(finalParameterValues[i], 20, 'f', 8).
            arg(sigma, 18).
            arg(m_adjustedSigmas[i] * RAD2DEG, 18, 'f', 8);
      }
      else {
        qStr = QString("%1%2%3%4%5%6\n").
            arg( parameterNamesList.at(i) ).
            arg((finalParameterValues[i] - m_corrections(i) * RAD2DEG), 17, 'f', 8).
            arg(m_corrections(i) * RAD2DEG, 21, 'f', 8).
            arg(finalParameterValues[i], 20, 'f', 8).
            arg(sigma, 18).
            arg("N/A", 18);
      }

      finalqStr += qStr;
    }

    return finalqStr;
  }


  /**
   * Access to parameters for CorrelationMatrix to use.
   *
   * @return @b QStringList Returns a QStringList of the names of the parameters
   */
  QStringList BundleObservation::parameterList() {
    return m_parameterNamesList;
  }


  /**
   * Access to image names for CorrelationMatrix to use.
   *
   * @return @b QStringList Returns a QStringList of the image names
   */
  QStringList BundleObservation::imageNames() {
    return m_imageNames;
  }
}
