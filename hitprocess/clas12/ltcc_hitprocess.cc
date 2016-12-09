// G4 headers
#include "G4MaterialPropertyVector.hh"
#include "Randomize.hh"

// gemc headers
#include "ltcc_hitprocess.h"

// C++ headers
#include <set>

// ccdb
#include <CCDB/Calibration.h>
#include <CCDB/Model/Assignment.h>
#include <CCDB/CalibrationGenerator.h>
using namespace ccdb;

static ltccConstants initializeLTCCConstants(int runno)
{
	// all these constants should be read from CCDB
	ltccConstants ltccc;


	// database
	ltccc.runNo = runno;
	ltccc.date       = "2016-03-15";
	if(getenv ("CCDB_CONNECTION") != NULL)
		ltccc.connection = (string) getenv("CCDB_CONNECTION");
	else
		ltccc.connection = "mysql://clas12reader@clasdb.jlab.org/clas12";

	ltccc.variation  = "main";
	auto_ptr<Calibration> calib(CalibrationGenerator::CreateCalibration(ltccc.connection));

	return ltccc;
}

map<string, double> ltcc_HitProcess :: integrateDgt(MHit* aHit, int hitn)
{
	map<string, double> dgtz;
	
	// we want to crash if identity doesn't have size 3
	vector<identifier> identity = aHit->GetId();
	int idsector  = identity[0].id;
	int idside    = identity[1].id;
	int idsegment = identity[2].id;
	int thisPid   = aHit->GetPID();
	
	trueInfos tInfos(aHit);
	
	// if anything else than a photon hits the PMT
	// the nphe is the particle id
	// and identifiers are negative
	// this should be changed, what if we still have a photon later?
	dgtz["sector"]  = -idsector;
	dgtz["side"]    = -idside;
	dgtz["segment"] = -idsegment;
	dgtz["nphe"]    = thisPid;
	dgtz["time"]    = tInfos.time;
	dgtz["hitn"]    = hitn;
	
	
	// if the particle is not an opticalphoton return bank filled with negative identifiers
	if(thisPid != 0)
		return dgtz;


	vector<int> tids = aHit->GetTIds();      // track ID at EACH STEP
	vector<int> pids = aHit->GetPIDs();      // particle ID at EACH STEP
	vector<double> Energies = aHit->GetEs(); // energy of the photon as it reach the pmt

	
	map<int, double> penergy;  // key is track id
	
	for(unsigned int s=0; s<tids.size(); s++)
	{
		// only insert the first step of each track
		// (i.e. if the map is empty
		if(penergy.find(tids[s]) == penergy.end())
			penergy[tids[s]] = Energies[s];
	}

	int narrived  = 0;
	int ndetected = 0;
	
	// If the detector corresponding to this hit has a material properties table with "Efficiency" defined:
	G4MaterialPropertiesTable* MPT = aHit->GetDetector().GetLogical()->GetMaterial()->GetMaterialPropertiesTable();
	G4MaterialPropertyVector* efficiency = NULL;
	
	bool gotefficiency = false;
	if( MPT != NULL )
	{
		efficiency = (G4MaterialPropertyVector*) MPT->GetProperty("EFFICIENCY");
		if( efficiency != NULL ) gotefficiency = true;
	}
	
	for( unsigned int iphoton = 0; iphoton<penergy.size(); iphoton++ )
	{
		//loop over all unique photons contributing to the hit:
		if( gotefficiency )
		{
			// If the material of this detector has a material properties table
			// with "EFFICIENCY" defined, then "detect" this photon with probability = efficiency
			bool outofrange = false;
			if( G4UniformRand() <= efficiency->GetValue( penergy[tids[iphoton]], outofrange ) )
				ndetected++;
			
			narrived++;
			
			if( verbosity > 4 )
			{
				cout << log_msg << " Found efficiency definition for material "
				<< aHit->GetDetector().GetLogical()->GetMaterial()->GetName()
				<< ": (Ephoton, efficiency)=(" << penergy[tids[iphoton]] << ", "
				<< ( (G4MaterialPropertyVector*) efficiency )->GetValue( penergy[tids[iphoton]], outofrange )
				<< ")" << endl;
			}
		}
		else
		{
			// No efficiency definition, "detect" all photons
			ndetected++;
		}
	}
	

	dgtz["sector"]  = idsector;
	dgtz["side"]    = idside;
	dgtz["segment"] = idsegment;
	dgtz["nphe"]    = narrived;
	dgtz["npheD"]   = ndetected;
	dgtz["time"]    = tInfos.time;
	dgtz["hitn"]    = hitn;

	
	return dgtz;

}


vector<identifier>  ltcc_HitProcess :: processID(vector<identifier> id, G4Step *step, detector Detector)
{
	id[id.size()-1].id_sharing = 1;
	return id;
}

void ltcc_HitProcess::initWithRunNumber(int runno)
{
	if(ltccc.runNo != runno) {
		cout << " > Initializing " << HCname << " digitization for run number " << runno << endl;
		ltccc = initializeLTCCConstants(runno);
		ltccc.runNo = runno;
	}
}


// - electronicNoise: returns a vector of hits generated / by electronics.
vector<MHit*> ltcc_HitProcess :: electronicNoise()
{
	vector<MHit*> noiseHits;

	// first, identify the cells that would have electronic noise
	// then instantiate hit with energy E, time T, identifier IDF:
	//
	// MHit* thisNoiseHit = new MHit(E, T, IDF, pid);

	// push to noiseHits collection:
	// noiseHits.push_back(thisNoiseHit)

	return noiseHits;
}


map< string, vector <int> >  ltcc_HitProcess :: multiDgt(MHit* aHit, int hitn)
{
	map< string, vector <int> > MH;
	return MH;
}


// - charge: returns charge/time digitized information / step
map< int, vector <double> > ltcc_HitProcess :: chargeTime(MHit* aHit, int hitn)
{
	map< int, vector <double> >  CT;

	return CT;
}

// - voltage: returns a voltage value for a given time. The inputs are:
// charge value (coming from chargeAtElectronics)
// time (coming from timeAtElectronics)
double ltcc_HitProcess :: voltage(double charge, double time, double forTime)
{
	return 0.0;
}

// this static function will be loaded first thing by the executable
ltccConstants ltcc_HitProcess::ltccc = initializeLTCCConstants(-1);





