#include "BSWAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


BSWAnalyzerSettings::BSWAnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL ),
	mBitRate( 9600 )
{
	mInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelInterface->SetTitleAndTooltip( "Serial", "Standard Bi-Spy-Wire" );
	mInputChannelInterface->SetChannel( mInputChannel );

	mBitRateInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mBitRateInterface->SetTitleAndTooltip( "Bit Rate (Bits/S)",  "Specify the bit rate in bits per second." );
	mBitRateInterface->SetMax( 6000000 );
	mBitRateInterface->SetMin( 1 );
	mBitRateInterface->SetInteger( mBitRate );

	AddInterface( mInputChannelInterface.get() );
	AddInterface( mBitRateInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mInputChannel, "Serial", false );
}

BSWAnalyzerSettings::~BSWAnalyzerSettings()
{
}

bool BSWAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface->GetChannel();
	mBitRate = mBitRateInterface->GetInteger();

	ClearChannels();
	AddChannel( mInputChannel, "Bi-Spy-Wire", true );

	return true;
}

void BSWAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel( mInputChannel );
	mBitRateInterface->SetInteger( mBitRate );
}

void BSWAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> mBitRate;

	ClearChannels();
	AddChannel( mInputChannel, "Bi-Spy-Wire", true );

	UpdateInterfacesFromSettings();
}

const char* BSWAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;
	text_archive << mBitRate;

	return SetReturnString( text_archive.GetString() );
}
