#ifndef BSW_ANALYZER_SETTINGS
#define BSW_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class BSWAnalyzerSettings : public AnalyzerSettings
{
public:
	BSWAnalyzerSettings();
	virtual ~BSWAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	
	Channel mInputChannel;
	U32 mBitRate;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mInputChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	mBitRateInterface;
};

#endif //BSW_ANALYZER_SETTINGS
