#include "BSWAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


BSWAnalyzerSettings::BSWAnalyzerSettings()

:	mSBWDIOChannel( UNDEFINED_CHANNEL ), 
	mSBWCLKChannel( UNDEFINED_CHANNEL ), 
	mCLKLowResetMicroSecs( 7 )
{
    mSBWDIOInterface.reset( new AnalyzerSettingInterfaceChannel() );
    mSBWDIOInterface->SetTitleAndTooltip( "SBWDIO", "Standard Bi-Spy-Wire Data In Out" );
    mSBWDIOInterface->SetChannel( mSBWDIOChannel );

    mSBWCLKInterface.reset( new AnalyzerSettingInterfaceChannel() );
    mSBWCLKInterface->SetTitleAndTooltip( "SBWCLK", "Standard Bi-Spy-Wire Clock" );
    mSBWCLKInterface->SetChannel( mSBWLCKChannel );


	mResettimeInterface.reset( new AnalyzerSettingInterfaceInteger() );
    mResettimeInterface->SetTitleAndTooltip( "Timeout", "Time SBWCLK is low to reset interface (in microsecs)." );
    mResettimeInterface->SetMax( 6000000 );
    mResettimeInterface->SetMin( 1 );
    mResettimeInterface->SetInteger( mRessettime );

	AddInterface( mSBWDIOInterface.get() );
    AddInterface( mSBWCLKInterface.get() );

	AddInterface( mResettimeInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();

    AddChannel( mSBWDIOChannel, "SBWDIO", false );
    AddChannel( mSBWTCKChannel, "SBWCLK", false );

}

BSWAnalyzerSettings::~BSWAnalyzerSettings()
{
}

bool BSWAnalyzerSettings::SetSettingsFromInterfaces()
{

    if( mSBWDIOInterface.GetChannel() == UNDEFINED_CHANNEL )
    {
        SetErrorText( "Please select an input for the channel 1." );
        return false;
    }

    if( mSBWCLKInterface.GetChannel() == UNDEFINED_CHANNEL )
    {
        SetErrorText( "Please select an input for the channel 2." );
        return false;
    }

    mSBWDIOChannel = mSBWDIOInterface->GetChannel();
    mSBWCLKChannel = mSBWCLKInterface->GetChannel();

    if( mSBWDIOChannel == mSBWCLKChannel )
    {
        SetErrorText( "Please select different inputs for the channels." );
        return false;
    }

	mResettime = mResettimeInterface->GetInteger();

	ClearChannels();
    AddChannel( mSBWDIOChannel, "SBWDIO", false );
    AddChannel( mSBWTCKChannel, "SBWCLK", false );

	return true;
}

void BSWAnalyzerSettings::UpdateInterfacesFromSettings()
{

    mSBWDIOInterface->SetChannel( mSBWDIOChannel );
    mSBWCLKInterface->SetChannel( mSBWCLKChannel );

	mResettimeInterface->SetInteger( mResettime );
}

void BSWAnalyzerSettings::LoadSettings( const char* settings )
{

        // TODO
    /*
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> mBitRate;

	ClearChannels();
	AddChannel( mInputChannel, "Bi-Spy-Wire", true );

	UpdateInterfacesFromSettings();
    */
}

const char* BSWAnalyzerSettings::SaveSettings()
{

       // TODO

    /*
	SimpleArchive text_archive;

	text_archive << mInputChannel;
	text_archive << mBitRate;

	return SetReturnString( text_archive.GetString() );
    */
       return "";
}
