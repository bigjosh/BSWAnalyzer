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

    U32 mSampleRateHz = GetSampleRate();

    // Resettime is in microseconds, so we divide by 1,000,000 to get seconds.
    U32 TCK_samples_to_reset = U32( double( mSampleRateHz ) / ( double( mSettings->mResettime ) / 1000000.0 ) );

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

    U64 lastCLKhighEdgeFrame = 0;

    // We need to start with a clean entry point, so wait for a time when TCK is high longer than reset period

    // Jump to first high edge
    if( mBSWTCK->GetBitState() == BIT_LOW )
        mBSWTCK->AdvanceToNextEdge();

    U64 last_rising_clk = mBSWTCK->GetSampleNumber();

    // Advance to falling edge
    mBSWTCK->AdvanceToNextEdge();

    while( ( mBSWTCK->GetSampleNumber() - last_rising_clk ) <  TCK_samples_to_reset )
    {


        mBSWTCK->AdvanceToNextEdge();
        // Clock is high

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

        // Result we will put into the frame is all goes well.
        U64 result = 0;

        auto readNextBit = [ & ]( const U8 bitmask )
        {
            // Assumes you are at falling edge
            // Reads a bit. 
            // Advances to next rising edge 
            // Advances to next falling edge
            // Returns true if hightime exceeded timeout

            U64 falling_edge_sample = mBSWTCK->GetSampleNumber();

            // Jump to clock to next rising edge
            mBSWTCK->AdvanceToNextEdge();

            last_rising_clk = mBSWTCK->GetSampleNumber();

            // Advance to next falling edge
            mBSWTCK->AdvanceToNextEdge();

            if( falling_edge_sample - last_rising_clk >= TCK_samples_to_reset )
            {
                // The clock was high too long, so we have to abort this frame

                // Add a marker to show we are restarting a frame
                mResults->AddMarker( falling_edge_sample, AnalyzerResults::Start, mSettings->mSBWTCKChannel );

                return true;
            }

            // Now we know that no reset happened, we can parse the data from the initial falling edge

            // Move data channel to the falling edge
            mBSWDIO->AdvanceToAbsPosition( falling_edge_sample );

            // Add a marker to show we are sampling a bit
            mResults->AddMarker( falling_edge_sample, AnalyzerResults::Dot, mSettings->mSBWDIOChannel );

            // Read the state from the data line and fold into the result

            if( mBSWDIO->GetBitState() == BIT_HIGH )
            {
                result |= ( static_cast<unsigned long long>( 1 ) << bitmask );
            }

            return false;
        };

        // possible begining of frame
        U64 starting_sample = mBSWTCK->GetSampleNumber();

        if( !readNextBit( 1 << 2 ) )
        {
            // We are at the falleing edge of TMS
           
            // Advance to next rising clock
            mBSWTCK->AdvanceToNextEdge();

            if( !readNextBit( 1 << 1 ) )
            {
                // We are at the falling edge of DO
                // Advance to next rising clock
                mBSWTCK->AdvanceToNextEdge();

                if( !readNextBit( 1 << 1 ) )
                {
                    // We are at the falling edge of DI
                    // We got a full frame with all 3 signals.

                    // End with the falling edge of the 3rd bit
                    U64 ending_sample = mBSWTCK->GetSampleNumber();

                    Frame frame;
                    frame.mData1 = result;
                    frame.mFlags = 0;
                    frame.mStartingSampleInclusive = starting_sample;
                    frame.mEndingSampleInclusive = ending_sample;

                    mResults->AddFrame( frame );
                    mResults->CommitResults();
                }
            }
        }

		ReportProgress( mBSWTCK->GetSampleNumber() );

        // Advance to rising edge, ready to start a new frame
        mBSWTCK->AdvanceToNextEdge();
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