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
    TCK_samples_to_reset = U32( double( mSampleRateHz ) / ( double( mSettings->mResettime ) / 1000000.0 ) );

    mBSWTCK = GetAnalyzerChannelData( mSettings->mSBWTCKChannel );
    mBSWDIO = GetAnalyzerChannelData( mSettings->mSBWDIOChannel );

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

    U32 lastCLKhighEdgeFrame = 0;

    // We need to start with a clean entry point, so wait for a time when TCK is high longer than reset period

    // Jump to first high edge
    if( mBSWTCK->GetBitState() == BIT_LOW )
        mBSWTCK->AdvanceToNextEdge();

    // Here we know clock is high

    while( ( ( mBSWTCK->GetSampleNumber() ) - lastCLKhighEdgeFrame ) < TCK_samples_to_reset )
    {
        lastCLKhighEdgeFrame = mBSWTCK->GetSampleNumber();
        mBSWTCK->AdvanceToNextEdge();
        // Clock is low
        mBSWTCK->AdvanceToNextEdge();
        // Clock is high
    }


    for( ;; )
    {
        // OK, clock is now high and has been high long enough to reset
        lastCLKhighEdgeFrame = mBSWTCK->GetSampleNumber();

        U32 starting_sample = mBSWTCK->GetSampleNumber();

        // Add a marker to show we are starting a frame
        // This is nice becuase it will show the reset even if this frame ends up not getting properly decoded
        mResults->AddMarker( starting_sample, AnalyzerResults::Start, mSettings->mSBWTCKChannel );

        // Result we will put into the frame is all goes well.
        U32 result = 0;

        auto readNextBit = [ & ]( const U8 bitmask )
        {
            // Reads a bit. Assumes clock is high. Returns at the falling edge.
            // Returns flase if bit was read, true if hightime exceeded timeout

            U32 clock_rise_sample = mBSWTCK->GetSampleNumber(); // When did clock rise?

            // Jump to clock falling edge
            mBSWTCK->AdvanceToNextEdge();

            U32 falling_edge_sample = mBSWTCK->GetSampleNumber();


            if( falling_edge_sample - clock_rise_sample >= TCK_samples_to_reset )
            {
                // The clock was high too long, so we have to abort this frame
                return true;
            }


            // Move data channel to the falling edge
            mBSWDIO->AdvanceToAbsPosition( falling_edge_sample );


            // Add a marker to show we are sampling a bit
            mResults->AddMarker( falling_edge_sample, AnalyzerResults::Dot, mSettings->mSBWDIOChannel );

            // Read the state from the data line and fold into the result

            if( mBSWDIO->GetBitState() == BIT_HIGH )
            {
                result |= ( 1 << bitmask );
            }

            return false;
        };

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
                    U32 ending_sample = mBSWTCK->GetSampleNumber();

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