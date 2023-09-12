#ifndef BSW_ANALYZER_H
#define BSW_ANALYZER_H

#include <Analyzer.h>
#include "BSWAnalyzerResults.h"
#include "BSWSimulationDataGenerator.h"

class BSWAnalyzerSettings;
class ANALYZER_EXPORT BSWAnalyzer : public Analyzer2
{
public:
	BSWAnalyzer();
	virtual ~BSWAnalyzer();

	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	std::auto_ptr< BSWAnalyzerSettings > mSettings;
	std::auto_ptr< BSWAnalyzerResults > mResults;


	BSWSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //BSW_ANALYZER_H
