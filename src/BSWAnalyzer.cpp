#define LOGIC2

#include "BSWAnalyzer.h"
#include "BSWAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <AnalyzerResults.h>        // Apparently needed for FrameV2

BSWAnalyzer::BSWAnalyzer()
:	Analyzer2(),  
	mSettings( new BSWAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
    UseFrameV2();             // Sadly following the docs dows not seem to work. 
}

BSWAnalyzer::~BSWAnalyzer()
{
	KillThread();
}

void BSWAnalyzer::SetupResults()
{
	mResults.reset( new BSWAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mSBWTCKChannel );

}


void BSWAnalyzer::WorkerThread()
{

    const U32 samples_per_sec = GetSampleRate();

    const U32 microsecs_per_sec = 1000000UL;

    const U32 microsecs_per_reset = mSettings->mResettime;

    const U32 samples_per_microsec = samples_per_sec / microsecs_per_sec;

    const U32 samples_per_reset = samples_per_microsec * microsecs_per_reset;

    AnalyzerChannelData* mBSWTCK = GetAnalyzerChannelData( mSettings->mSBWTCKChannel );
    AnalyzerChannelData* mBSWDIO = GetAnalyzerChannelData( mSettings->mSBWDIOChannel );


    /*
    * 
    * Test code

    while( 1 )
    {

    // Jump to first high edge
        if( mBSWTCK->GetBitState() == BIT_LOW )
            mBSWTCK->AdvanceToNextEdge();

        U64 start = mBSWTCK->GetSampleNumber();

        mResults->AddMarker( start, AnalyzerResults::Start, mSettings->mSBWTCKChannel );

        mBSWTCK->AdvanceToNextEdge();

        U64 end = mBSWTCK->GetSampleNumber();

        mBSWDIO->AdvanceToAbsPosition( end );

        mResults->AddMarker( end, AnalyzerResults::Dot, mSettings->mSBWDIOChannel );


        Frame frame;
        frame.mData1 = mBSWDIO->GetBitState() == BIT_HIGH ? 1 : 0;
        frame.mFlags = 0;
        frame.mStartingSampleInclusive = start;
        frame.mEndingSampleInclusive = end;

        mResults->AddFrame( frame );
        mResults->CommitResults();

        ReportProgress( end );
            
    }

    */



    // We need to start with a clean entry point, so wait for a time when TCK is high longer than reset period

    // Jump to first high edge
    if( mBSWTCK->GetBitState() == BIT_LOW )
    {
        mBSWTCK->AdvanceToNextEdge();
        mResults->AddMarker( mBSWTCK->GetSampleNumber(), AnalyzerResults::UpArrow, mSettings->mSBWTCKChannel );
    }


    // Frame always starts on first falling edge
    U64 frame_start;

    // The result we will put into the frame if all goes well.
    U64 result;

    // Which bit are we currently reading?
    // Here we decide that we do not need to see a sync at the very begining of a capture.
    // Is this correct? I guess you can always use the trigger to make sure the capture starts with a snyc. 
    unsigned current_bit = 0;

    // Enter this main loop on rising clk edge

    for( ;; )
    {

        U64 last_rising_clk = mBSWTCK->GetSampleNumber();

        // Advance to falling edge
        mBSWTCK->AdvanceToNextEdge();

        // Low

        U64 falling_edge = mBSWTCK->GetSampleNumber();

        if( ( falling_edge - last_rising_clk ) >= samples_per_reset )
        {
            // We were high long enough to trigger a reset
            current_bit = 0;
            // Show the reset with a start marker
            mResults->AddMarker( falling_edge, AnalyzerResults::Start, mSettings->mSBWTCKChannel );
        }

        if(current_bit==0) {
            // Begining of a new frame
            result = 0;
            frame_start = falling_edge;        
        } 

        // Move data channel to the falling edge
        mBSWDIO->AdvanceToAbsPosition( falling_edge );

        // Don't show a marker on the DO falling edge since it doesnot matter. We will mark the rising edge below. 
        if( current_bit != 2 )
        {
            // Add a marker to show we are sampling a data bit
            mResults->AddMarker( falling_edge, AnalyzerResults::Dot, mSettings->mSBWDIOChannel );
        }

        // Read the state from the data line and fold into the result

        if( mBSWDIO->GetBitState() == BIT_HIGH )
        {
            result |= ( static_cast<unsigned long long>( 1 ) << current_bit );
        }

        current_bit++;

        mBSWTCK->AdvanceToNextEdge();

        // High 

        if( current_bit == 3 )      // Are we on the final TDO bit? 
        {

            U64 final_rising_edge = mBSWTCK->GetSampleNumber();

            // TDO bit is different. We also this one on the rising edge. 

            // Move data channel to the rising edge
            mBSWDIO->AdvanceToAbsPosition( final_rising_edge);


            // Read the state from the data line and fold into the result
            // Add a marker to show we are sampling a data bit
            mResults->AddMarker( final_rising_edge, AnalyzerResults::Dot, mSettings->mSBWDIOChannel );

            if( mBSWDIO->GetBitState() == BIT_HIGH )
            {
                result |= ( static_cast<unsigned long long>( 1 ) << current_bit );
            }

            // Completed a frame

            // End with the falling edge of the 3rd bit

            Frame frame;
            frame.mData1 = result;
            frame.mFlags = 0;
            frame.mStartingSampleInclusive = frame_start;
            frame.mEndingSampleInclusive = final_rising_edge;       // The bubble will span from the initial falling edge where we sampled TMS to the final rising edge where we sample TDO
            mResults->AddFrame( frame );

            FrameV2 frame_v2;
            frame_v2.AddBoolean( "tms", ( frame.mData1 & ( static_cast<unsigned long long>( 1 ) << 0 ) ) != 0  );
            frame_v2.AddBoolean( "tdi", ( frame.mData1 & ( static_cast<unsigned long long>( 1 ) << 1 ) ) != 0 );
            frame_v2.AddBoolean( "tdo", ( frame.mData1 & ( static_cast<unsigned long long>( 1 ) << 3 ) ) != 0 );
            mResults->AddFrameV2( frame_v2, "jtag", frame.mStartingSampleInclusive, frame.mEndingSampleInclusive );

            mResults->CommitResults();

            result=0;
            current_bit = 0;

        }


        // High


        // Here we could test if the low was low too long, but what would we do?
        // Docs seems to say that the target SBW interface just stops if low is low too long, so will
        // need a reset anyway, and at least here you can see what the host is doing. 


		ReportProgress( mBSWTCK->GetSampleNumber() );

    }
       
}

bool BSWAnalyzer::NeedsRerun()
{
	return false;
}

U32 BSWAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 BSWAnalyzer::GetMinimumSampleRateHz()
{
	return mSettings->mResettime * 4;
}

const char* BSWAnalyzer::GetAnalyzerName() const
{
	return "Bi-Spy-Wire";
}

const char* GetAnalyzerName()
{
	return "Bi-Spy-Wire";
}

Analyzer* CreateAnalyzer()
{
	return new BSWAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}