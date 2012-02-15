/*
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 */
#include <cstdio>

#include <be_finger_an2kview.h>
#include <be_feature_an2k7minutiae.h>
#include <be_memory_autobuffer.h>
#include <be_io_utility.h>

using namespace BiometricEvaluation;

BiometricEvaluation::Feature::AN2K7Minutiae::AN2K7Minutiae(
    const std::string &filename,
    int recordNumber)
    throw (Error::DataError, Error::FileError)
{

	if (!IO::Utility::fileExists(filename))
		throw (Error::FileError("File not found."));

	FILE *fp = std::fopen(filename.c_str(), "rb");
	if (fp == NULL)
		throw (Error::FileError("Could not open file."));

	uint64_t sz = IO::Utility::getFileSize(filename);
	Memory::uint8Array buf(sz);
	if (fread(buf, 1, sz, fp) != sz) {
		fclose(fp);
		throw Error::FileError("Could not read AN2K file");
	}
        fclose(fp);
	
	readType9Record(buf, recordNumber);
}

BiometricEvaluation::Feature::MinutiaeFormat::Kind
BiometricEvaluation::Feature::AN2K7Minutiae::getFormat() const
{
	return (MinutiaeFormat::AN2K7);
}

BiometricEvaluation::Feature::AN2K7Minutiae::AN2K7Minutiae(
    Memory::uint8Array &buf,
    int recordNumber)
    throw (Error::DataError)
{
	readType9Record(buf, recordNumber);
}

BiometricEvaluation::Feature::AN2K7Minutiae::FingerprintReadingSystem
BiometricEvaluation::Feature::AN2K7Minutiae::
getOriginatingFingerprintReadingSystem() const
    throw (Error::ObjectDoesNotExist)
{
	if (_ofr.name.empty())
		throw Error::ObjectDoesNotExist();
	
	return (_ofr);
}

BiometricEvaluation::Feature::AN2K7Minutiae::PatternClassificationSet
BiometricEvaluation::Feature::AN2K7Minutiae::getPatternClassificationSet() const
{
	return (_fpc);
}

Feature::MinutiaPointSet
BiometricEvaluation::Feature::AN2K7Minutiae::getMinutiaPoints() const
{
	return (_minutiaPointSet);
}

Feature::RidgeCountItemSet
BiometricEvaluation::Feature::AN2K7Minutiae::getRidgeCountItems() const
{
	return (_ridgeCountItemSet);
}

Feature::CorePointSet
BiometricEvaluation::Feature::AN2K7Minutiae::getCores() const
{
	return (_corePointSet);
}

Feature::DeltaPointSet
BiometricEvaluation::Feature::AN2K7Minutiae::getDeltas() const
{
	return (_deltaPointSet);
}


BiometricEvaluation::Feature::AN2K7Minutiae::PatternClassification::Entry::Entry(
    bool standard,
    std::string code) :
    standard(standard),
    code(code)
{

}

/******************************************************************************/
/* Local functions.                                                           */
/******************************************************************************/

static void
convertXYT(unsigned char *value, Feature::MinutiaPoint *mp)
{
        char buf[5];
	buf[4] = '\0';
        Image::Coordinate coord;
        for (int i = 0; i < 4; i++)
                buf[i] = value[i];
        mp->coordinate.x = atoi(buf);
        for (int i = 4; i < 8; i++)
                buf[i - 4] = value[i];
        mp->coordinate.y = atoi(buf);
	buf[3] = '\0';
        for (int i = 8; i < 11; i++)
                buf[i - 8] = value[i];
        mp->theta = atoi(buf);
}

static void
extractRidge(
    const unsigned char *str,
    int *point,
    int *distance)
    throw (Error::DataError)
{
	/* Copy the string */
	int len = strlen((char*)str);
	unsigned char* copy = new unsigned char[len + 1];
	strncpy((char *)copy, (char *)str, len + 1);

	/* Split the copy */
	int i;
	for (i = 1; i < len; i++) {
		if (copy[i] == ',') {
			copy[i] = '\0';
			break;
		}
	}
	
	/* Make sure it split */
	if (i == (len - 1))
		throw Error::DataError("Invalid format for MRC ridge data");
	
	/* Assign the two integers */
	*point = atoi((char*)copy);
	*distance = atoi((char*)copy + i + 1);
	delete [] copy;
}

static void
readMRC(
    const RECORD *type9,
    Feature::MinutiaPointSet &mps,
    Feature::RidgeCountItemSet &rcs,
    bool hasRDG)
    throw (Error::DataError)
{
	FIELD *field;
	int idx;

	/* Number of minutiae */
	if (lookup_ANSI_NIST_field(&field, &idx, MIN_ID, type9) == FALSE)
		throw Error::DataError("Field MIN not found");
	int count = atoi((char*)field->subfields[0]->items[0]->value);

	/* Minutiae and Ridge Count data */
	if (lookup_ANSI_NIST_field(&field, &idx, MRC_ID, type9) == FALSE)
		throw Error::DataError("Field MRC not found");	
	for (int i = 0; i < count; i++) {
		Feature::MinutiaPoint mp;
		
		int numItems = field->subfields[i]->num_items;
		
		/* Get required fields */
		mp.index =
		    atoi((char*)field->subfields[i]->items[0]->value);
		convertXYT(field->subfields[i]->items[1]->value, &mp);

		/* Quality field present */
		if (numItems >= 3) {
			mp.has_quality = true;
			mp.quality = (unsigned int)atoi(
			    (char*)field->subfields[i]->items[2]->value);
		} else {
			mp.has_quality = false;
		}
		
		/* Minutiae type present */
		if (numItems >= 4) {
			mp.type = (Feature::MinutiaeType::Kind)atoi((char*)
			    field->subfields[i]->items[3]->value);
			mp.has_type = true;
		} else {
			mp.has_type = false;
		}
		mps.push_back(mp);
		
		/* The remaining information is ridge count data */
		if (hasRDG) {
			if (numItems < 5)
				throw Error::DataError("Corrupt Type-9");
			for (int j = 4; j < numItems; j++) {
				Feature::RidgeCountItem ridge;
				ridge.index_one = mp.index;
				extractRidge(
				    field->subfields[i]->items[j]->value,
				    &ridge.index_two, 
				    &ridge.count);
				rcs.push_back(ridge);
			}
		}
	}
}

std::ostream&
BiometricEvaluation::Feature::operator<< (std::ostream &s,
    const Feature::AN2K7Minutiae::EncodingMethod::Kind& mem)
{
	string str;
	switch (mem) {
	case Feature::AN2K7Minutiae::EncodingMethod::Automatic:
		str = "Automatic (no possible human interaction)"; break;
	case Feature::AN2K7Minutiae::EncodingMethod::AutomaticUnedited:
		str = "Automatic (editing possible, but not performed)"; break;
	case Feature::AN2K7Minutiae::EncodingMethod::AutomaticEdited:
		str = "Automatic (editing possible and performed)"; break;
	case Feature::AN2K7Minutiae::EncodingMethod::Manual:
		str = "Manual"; break;
	}
	
	return (s << str);
}

BiometricEvaluation::Finger::PatternClassification::Kind
BiometricEvaluation::Feature::AN2K7Minutiae::convertPatternClassification(
    const char *fpc)
    throw (Error::DataError)
{
	string str(fpc);
	if (str == "PA")
		return (Finger::PatternClassification::PlainArch);
	else if (str == "TA")
		return (Finger::PatternClassification::TentedArch);
	else if (str == "RL")
		return (Finger::PatternClassification::RadialLoop);
	else if (str == "UL")
		return (Finger::PatternClassification::UlnarLoop);
	else if (str == "PW")
		return (Finger::PatternClassification::PlainWhorl);
	else if (str == "CP")
		return (Finger::PatternClassification::CentralPocketLoop);
	else if (str == "DL")
		return (Finger::PatternClassification::DoubleLoop);
	else if (str == "AW")
		return (Finger::PatternClassification::AccidentalWhorl);
	else if (str == "WN")
		return (Finger::PatternClassification::Whorl);
	else if (str == "RS")
		return (Finger::PatternClassification::RightSlantLoop);
	else if (str == "LS") 
		return (Finger::PatternClassification::LeftSlantLoop);
	else if (str == "SR")
		return (Finger::PatternClassification::Scar);
	else if (str == "XX")
		return (Finger::PatternClassification::Amputation);
	else if (str == "UN")
		return (Finger::PatternClassification::Unknown);
	else
		throw Error::DataError("Invalid value for "
		    "pattern classification");	
}

BiometricEvaluation::Finger::PatternClassification::Kind
BiometricEvaluation::Feature::AN2K7Minutiae::convertPatternClassification(
    const PatternClassification::Entry &entry)
    throw (Error::DataError)
{
	if (entry.standard == false)
		throw Error::DataError("Non-standard pattern classification");
		
	return (convertPatternClassification(entry.code.c_str()));
}

BiometricEvaluation::Feature::AN2K7Minutiae::EncodingMethod::Kind
BiometricEvaluation::Feature::AN2K7Minutiae::convertEncodingMethod(
    const char *mem)
    throw (Error::DataError)
{
	string str(mem);
	if (str == "A") return (EncodingMethod::Automatic);
	else if (str == "U") return (EncodingMethod::AutomaticUnedited);
	else if (str == "E") return (EncodingMethod::AutomaticEdited);
	else if (str == "M") return (EncodingMethod::Manual);
	else throw Error::DataError("Invalid value for minutiae encoding "
	    "method");
}

std::ostream&
BiometricEvaluation::Feature::operator<<(
    std::ostream &s,
    const Feature::AN2K7Minutiae::FingerprintReadingSystem &frs)
{
	s << "Name: " << frs.name << endl;
	s << "Method: " << frs.method;
	if (frs.equipment.empty() == false)
		s << endl << "Equipment ID: " << frs.equipment;

	return (s);
}

BiometricEvaluation::Image::Coordinate
BiometricEvaluation::Feature::AN2K7Minutiae::convertCoordinate(
    const char *str,
    bool calculateDistance)
    throw (Error::DataError)
{
	if (strlen(str) != 8)
		throw Error::DataError("Coordinate encoding of invalid size");
		
	char x[5], y[5];
	for (int i = 0; i < 4; i++) x[i] = str[i];
	for (int i = 4; i < 8; i++) y[i - 4] = str[i];
	x[4] = y[4] = '\0';

	Image::Coordinate ret(atoi(x), atoi(y));
	
	if (calculateDistance) {
		static Image::Coordinate origin(0, 0, 0.0, 0.0);
		ret.xDistance = Image::distance(ret, origin);
		ret.yDistance = Image::distance(ret, origin);
	}
	
	return (ret);
}

/******************************************************************************/
/* Private functions.                                                         */
/******************************************************************************/

void
BiometricEvaluation::Feature::AN2K7Minutiae::readType9Record(
    Memory::uint8Array &buf,
    int recordNumber)
    throw (Error::DataError)
{
	Memory::AutoBuffer<ANSI_NIST> an2k =
	    Memory::AutoBuffer<ANSI_NIST>(&alloc_ANSI_NIST,
		&free_ANSI_NIST, &copy_ANSI_NIST);

	AN2KBDB bdb;
	INIT_AN2KBDB(&bdb, buf, buf.size());
	if (scan_ANSI_NIST(&bdb, an2k) != 0)
		throw Error::DataError("Could not read complete AN2K record");

	/*
	 * Find the requested Type-9 in the file, throwing an exception
	 * if not present. The first record in an AN2K file is always
	 * the Type-1, so skip that one.
	 */
	RECORD *type9 = NULL;
	for (int i = 1; i < an2k->num_records; i++) {
		if (an2k->records[i]->type == TYPE_9_ID) {
			if (i == recordNumber) {
				type9 = an2k->records[i];
				break;
			}
		}
	}
	if (type9 == NULL)
		throw (Error::DataError(
		    "Could not find requested Type-9 in AN2K record"));

	/*********************************************************************/
	/* Required Fields.                                                  */
	/*********************************************************************/
	FIELD *field;
	int idx;

	if (lookup_ANSI_NIST_field(&field, &idx, FGP2_ID, type9) == FALSE)
		throw Error::DataError("Field FGP not found");
	_fgp = Finger::AN2KView::populateFGP(field);
	
	if (lookup_ANSI_NIST_field(&field, &idx, FPC_ID, type9) == FALSE)
		throw Error::DataError("Field FPC not found");
	for (int i = 0; i < field->num_subfields; i++)
		_fpc.push_back(PatternClassification::Entry(
		    (string((char*)field->subfields[i]->items[0]->
		    value) == "T"), (char *)field->subfields[i]->
		    items[1]->value));

	if (lookup_ANSI_NIST_field(&field, &idx, RDG_ID, type9) == FALSE)
		throw Error::DataError("Field RDG not found");
	bool hasRDG = atoi((char*)field->subfields[0]->items[0]->value);

	readMRC(type9, _minutiaPointSet, _ridgeCountItemSet, hasRDG);

	/*********************************************************************/
	/* Optional Fields.                                                  */
	/*********************************************************************/
	
	/* Originating Fingerprint Reader System */
	if (lookup_ANSI_NIST_field(&field, &idx, OFR_ID, type9) == TRUE) {
		_ofr.name = (char*)field->subfields[0]->items[0]->value;
		_ofr.method = convertEncodingMethod((char*)field->
		    subfields[0]->items[1]->value);

		/* Equipment ID is optional */
		if (field->subfields[0]->num_items == 3)
			_ofr.equipment = (char*)field->subfields[0]->
			    items[2]->value;
	}

	/* Cores */
	if (lookup_ANSI_NIST_field(&field, &idx, CRP_ID, type9) == TRUE)
		for (int i = 0; i < field->num_subfields; i++)
			_corePointSet.push_back(CorePoint(convertCoordinate(
			    (char *)field->subfields[i]->items[0]->value)));

	/* Deltas */
	if (lookup_ANSI_NIST_field(&field, &idx, DLT_ID, type9) == TRUE)
		for (int i = 0; i < field->num_subfields; i++)
			_deltaPointSet.push_back(DeltaPoint(convertCoordinate(
			    (char *)field->subfields[i]->items[0]->value,
			    false)));

}
