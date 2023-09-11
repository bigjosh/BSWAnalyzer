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


    Channel mSBWDIOChannel;		// Data
    Channel mSBWTCKChannel;		// Clock

	U32 mResettime;				// Typically 7us

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel > mSBWDIOInterface;
	std::auto_ptr< AnalyzerSettingInterfaceChannel > mSBWCLKInterface;
    std::auto_ptr< AnalyzerSettingInterfaceInteger>  mResettimeInterface;

};

#endif //BSW_ANALYZER_SETTINGS
