#ifndef BSW_SIMULATION_DATA_GENERATOR
#define BSW_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class BSWAnalyzerSettings;

class BSWSimulationDataGenerator
{
public:
	BSWSimulationDataGenerator();
	~BSWSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, BSWAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	BSWAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void CreateSerialByte();
	std::string mSerialText;
	U32 mStringIndex;

	SimulationChannelDescriptor mSerialSimulationData;

};
#endif //BSW_SIMULATION_DATA_GENERATOR