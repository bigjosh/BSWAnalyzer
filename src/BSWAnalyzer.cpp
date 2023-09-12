#include "BSWAnalyzer.h"
#include "BSWAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

BSWAnalyzer::BSWAnalyzer()
:	Analyzer2(),  
	mSettings( new BSWAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
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

    // High

    U64 last_rising_clk = mBSWTCK->GetSampleNumber();

    // Advance to falling edge
    mBSWTCK->AdvanceToNextEdge();

    // Low

    // Sync by Lookink for a falled edge that has a long enough high before it

    while( ( mBSWTCK->GetSampleNumber() - last_rising_clk ) <  samples_per_reset )
    {
        // Low

        mResults->AddMarker( mBSWTCK->GetSampleNumber(), AnalyzerResults::ErrorDot, mSettings->mSBWTCKChannel );
    
        mBSWTCK->AdvanceToNextEdge();
        // Clock is high

        mResults->AddMarker( mBSWTCK->GetSampleNumber(), AnalyzerResults::UpArrow, mSettings->mSBWTCKChannel );


        last_rising_clk = mBSWTCK->GetSampleNumber();

        mBSWTCK->AdvanceToNextEdge();
        // Clock is low

    }

    // Clock is low

    // OK we are at the falling edge of a high that is longer than the reset period, so this is a good place to start

    // Add a marker to show we are restarting a frame (this is actually the initial sync)
    mResults->AddMarker( mBSWTCK->GetSampleNumber(), AnalyzerResults::Start, mSettings->mSBWTCKChannel );


    for( ;; )
    {
        // OK, clock is now low and we are at the start of a new frame

        // The result we will put into the frame if all goes well.
        U64 result = 0;

        auto readNextBit = [ & ]( const U8 bitmask )
        {
            // Assumes you are at falling edge
            // Reads a bit. 
            // Advances to next rising edge 
            // Advances to next falling edge
            // Returns true if hightime of the most recent high exceeded the timeout (so should restart frame)

            U64 falling_edge_sample = mBSWTCK->GetSampleNumber();

            // Read data bit

            // Move data channel to the falling edge
            mBSWDIO->AdvanceToAbsPosition( falling_edge_sample );

            // Add a marker to show we are sampling a bit
            mResults->AddMarker( falling_edge_sample, AnalyzerResults::Dot, mSettings->mSBWDIOChannel );

            // Read the state from the data line and fold into the result

            if( mBSWDIO->GetBitState() == BIT_HIGH )
            {
                result |= ( static_cast<unsigned long long>( 1 ) << bitmask );
            }


            // Now we have to step forard to the next falling edge

            // Jump to clock to next rising edge
            mBSWTCK->AdvanceToNextEdge();

            // High

            last_rising_clk = mBSWTCK->GetSampleNumber();

            mBSWTCK->AdvanceToNextEdge();

            // Low


            if( mBSWTCK->GetSampleNumber() - last_rising_clk >= samples_per_reset )
            {
                // The clock was high too long, so we have to abort this frame

                // Add a marker to show we are restarting a frame
                mResults->AddMarker( falling_edge_sample, AnalyzerResults::Start, mSettings->mSBWTCKChannel );

                return true;
            }

            // Now we know that no reset happened, we can parse the data from the initial falling edge


            return false;
        };

        // possible begining of frame
        U64 frame_start_sample = mBSWTCK->GetSampleNumber();

        if( !readNextBit( 1 << 2 ) )
        {
            // We are at the falling edge of TMS
           
            if( !readNextBit( 1 << 1 ) )
            {
                // We are at the falling edge of DO
           
                if( !readNextBit( 1 << 0 ) )
                {

                    // We are at the falling edge of DI
                    // We got a full frame with all 3 signals.

                    // End with the falling edge of the 3rd bit
                    U64 frame_end_sample = mBSWTCK->GetSampleNumber();

                    Frame frame;
                    frame.mData1 = result;
                    frame.mFlags = 0;
                    frame.mStartingSampleInclusive = frame_start_sample;
                    frame.mEndingSampleInclusive = frame_end_sample;

                    mResults->AddFrame( frame );
                    mResults->CommitResults();
                }
            }
        }

        // Note that if any of the above readNextBit()'s found a reset, then we will enter the next loop iteration
        // on the falling edge after that long high and start reading a frame. 

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