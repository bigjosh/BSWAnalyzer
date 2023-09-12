#include "BSWAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "BSWAnalyzer.h"
#include "BSWAnalyzerSettings.h"
#include <iostream>
#include <fstream>

BSWAnalyzerResults::BSWAnalyzerResults( BSWAnalyzer* analyzer, BSWAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

BSWAnalyzerResults::~BSWAnalyzerResults()
{
}

char bitChar( const U64 x , unsigned place )
{
    return ( x & (  ((U64) 1) << place ) ? '1' : '0' );
}

void BSWAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	auto data = frame.mData1;

    char buf[ 64 ];
    ::snprintf( buf, 64, "TMS=%c, TDI=%c, TDO=%c",  bitChar(  data, 0) , bitChar( data , 1 ) , bitChar( data , 3 )  );
    AddResultString( buf );


	::snprintf( buf, 64, "MS=%c DI=%c DO=%c", bitChar( data, 0 ), bitChar( data, 1 ), bitChar( data, 3 ) );
    AddResultString( buf );


	::snprintf( buf, 64, "%c%c%c", bitChar( data, 0 ), bitChar( data, 1 ), bitChar( data, 3 ) );
    AddResultString( buf );

}

void BSWAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Value" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

		file_stream << time_str << "," << number_str << std::endl;

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void BSWAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
	Frame frame = GetFrame( frame_index );

	auto data = frame.mData1;
	ClearTabularText();

    // target content: [13] 0 , 1 , 0
    char buf[ 64 ];
    ::snprintf( buf, 64, "%c, %c, %c", bitChar( data, 0 ), bitChar( data, 1 ), bitChar( data, 3 ) );
    AddTabularText( buf );

#endif
}

void BSWAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	//not supported

}

void BSWAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	//not supported
}