#include "DspStft.h"
#include <math.h>
#include "kissfft/pv.h"

static const float PI = 3.1415926535897932384626433832795f;
static const float TWOPI = 6.283185307179586476925286766559f;

//=================================================================================================

DspStft::DspStft()
    : _bufferSize( 1024 ),
    _sampleRate( 44100 )
{
    AddInput_();
    AddInput_();
    AddInput_( "Sample Rate" );
    AddOutput_();
    AddOutput_();
}

//-------------------------------------------------------------------------------------------------

DspStft::~DspStft() {}

//=================================================================================================

void DspStft::Process_( DspSignalBus& inputs, DspSignalBus& outputs )
{
    _busyMutex.Lock();

    // 0. get current buffers (L & R)
    inputs.GetValue( 0, _inputBufL );
    inputs.GetValue( 1, _inputBufR );

    inputs.GetValue( "Sample Rate", _sampleRate );

    if ( _bufferSize != _inputBufL.size() )
    {
        _SetBufferSize( ( unsigned short ) _inputBufL.size() );
    }

    if ( _bufferSize != 0 )
    {
        // 1. build sigBufs: stored buffer + current buffer (L & R)
        _BuildSignalBuffers();

        // 2. replace stored buffer with current buffer (L & R)
        _StoreCurrentBuffers();

        // 3. apply window to sigbuf (L & R)
        _ApplyHanningWindows();

        // 4. do fft to sigbuf -> specbuf (L & R)
        _FftSignalBuffers();

        // 5. process specbuf (L & R)
        _ProcessSpectralBuffers();

        // 6. do ifft to specbuf -> sigbuf (L & R)
        _IfftSpectralBuffers();

        // 7. add first half of sigbuf to last buffer (L & R)
        for ( unsigned short i = 0; i < _bufferSize; i++ )
        {
            _outputBufL[ i ] = _mixBufL[ i ] + _sigBufL[ i ];
        }
        for ( unsigned short i = 0; i < _bufferSize; i++ )
        {
            _outputBufR[ i ] = _mixBufR[ i ] + _sigBufR[ i ];
        }

        // 8. replace current buffer with second half of sigbuf (L & R)
        for ( unsigned short i = 0; i < _bufferSize; i++ )
        {
            _mixBufL[ i ] = _sigBufL[ _bufferSize + i ];
        }
        for ( unsigned short i = 0; i < _bufferSize; i++ )
        {
            _mixBufR[ i ] = _sigBufR[ _bufferSize + i ];
        }

        // 9. output next buffer
        outputs.SetValue( 0, _outputBufL );
        outputs.SetValue( 1, _outputBufR );
    }

    _busyMutex.Unlock();
}

//=================================================================================================
void DspStft::_SetBufferSize( unsigned short bufferSize )
{
    _bufferSize = bufferSize;
    _InitBuffers();
    _BuildHanningLookup();
}

//-------------------------------------------------------------------------------------------------

void DspStft::_InitBuffers()
{
    _inputBufL.resize( _bufferSize ); // if we don't do this, input buffers might not be the same length
    _inputBufR.resize( _bufferSize );

    _prevInputBufL.resize( _bufferSize );
    _prevInputBufL.assign( _bufferSize, 0 );
    _prevInputBufR.resize( _bufferSize );
    _prevInputBufR.assign( _bufferSize, 0 );

    _outputBufL.resize( _bufferSize );
    _outputBufL.assign( _bufferSize, 0 );
    _outputBufR.resize( _bufferSize );
    _outputBufR.assign( _bufferSize, 0 );

    _sigBufL.resize( _bufferSize * 2 );
    _sigBufL.assign( _bufferSize * 2, 0 );
    _sigBufR.resize( _bufferSize * 2 );
    _sigBufR.assign( _bufferSize * 2, 0 );

    _specBufL.resize( _bufferSize * 4 );
    _specBufL.assign( _bufferSize * 4, 0 );
    _specBufR.resize( _bufferSize * 4 );
    _specBufR.assign( _bufferSize * 4, 0 );

    _mixBufL.resize( _bufferSize );
    _mixBufL.assign( _bufferSize, 0 );
    _mixBufR.resize( _bufferSize );
    _mixBufR.assign( _bufferSize, 0 );
}

//-------------------------------------------------------------------------------------------------

void DspStft::_BuildHanningLookup()
{
    unsigned short i;

    _hanningLookup.resize( _bufferSize * 2 ); // hanning window lookup table is double the length of sigbuf

    for ( i = 0; i < _bufferSize; i++ )
    {
        _hanningLookup[ i ] = 0.5f - ( float ) ( 0.5f * cos( i * PI / ( _bufferSize ) ) ); // store 1st half window
    }

    for ( i = 0; i < _bufferSize; i++ )
    {
        _hanningLookup[ _bufferSize + i ] = 0.5f - ( float ) ( 0.5f * cos( ( i * PI / ( _bufferSize ) ) + PI ) ); // store 2nd half window
    }
}

//-------------------------------------------------------------------------------------------------

void DspStft::_BuildSignalBuffers()
{
    // build left channel
    for ( unsigned short i = 0; i < _prevInputBufL.size(); i++ )
    {
        _sigBufL[ i ] = _prevInputBufL[ i ];
    }
    for ( unsigned short i = 0; i < _inputBufL.size(); i++ )
    {
        _sigBufL[ _bufferSize + i ] = _inputBufL[ i ];
    }

    // build right channel
    for ( unsigned short i = 0; i < _prevInputBufR.size(); i++ )
    {
        _sigBufR[ i ] = _prevInputBufR[ i ];
    }
    for ( unsigned short i = 0; i < _inputBufR.size(); i++ )
    {
        _sigBufR[ _bufferSize + i ] = _inputBufR[ i ];
    }
}

//-------------------------------------------------------------------------------------------------

void DspStft::_StoreCurrentBuffers()
{
    _prevInputBufL = _inputBufL;
    _prevInputBufR = _inputBufR;
}

//-------------------------------------------------------------------------------------------------

void DspStft::_ApplyHanningWindows()
{
    for ( unsigned short i = 0; i < _sigBufL.size(); i++ )
        _sigBufL[ i ] *= _hanningLookup[ i ]; //apply hanning window to left channel

    for ( unsigned short i = 0; i < _sigBufR.size(); i++ )
        _sigBufR[ i ] *= _hanningLookup[ i ]; //apply hanning window to right channel
}

//-------------------------------------------------------------------------------------------------

void DspStft::_FftSignalBuffers()
{
    fft_kiss( &_sigBufL[ 0 ], &_specBufL[ 0 ], _sigBufL.size() );
    fft_kiss( &_sigBufR[ 0 ], &_specBufR[ 0 ], _sigBufR.size() );
}

//-------------------------------------------------------------------------------------------------

void DspStft::_ProcessSpectralBuffers()
{
    float _frq, _magL, _phiL, _magR, _phiR; // frequency, magnitudes, and phases pulled from FFT result

                                            // 0Hz to Nyquist Frequency (bufferLength / 2)
    for ( size_t i = _specBufL.size() / 2; i > 0; i -= 2 )
    {
        //retrieve frequency
        _frq = ( float ) i * _sampleRate / ( float ) _specBufL.size();

        //retrieve magnitudes (L & R)
        _magL = 2.0f * sqrt( _specBufL[ i ] * _specBufL[ i ] + _specBufL[ i + 1 ] * _specBufL[ i + 1 ] );	//[i] = real [i+1] = imaginary
        _magR = 2.0f * sqrt( _specBufR[ i ] * _specBufR[ i ] + _specBufR[ i + 1 ] * _specBufR[ i + 1 ] );

        //retrieve phases (L & R)
        //atan(i,r) gives (-pi to +pi), so add pi and divide by 2pi to get (0 to 1), then shift left 0.25 (90deg) to convert cos phi to sin phi
        _phiL = ( ( atan2( _specBufL[ i + 1 ], _specBufL[ i ] ) + PI ) / TWOPI ) - 0.25f;
        _phiR = ( ( atan2( _specBufR[ i + 1 ], _specBufR[ i ] ) + PI ) / TWOPI ) - 0.25f;

        //convert phase to a value between 0 and 1 (L & R)
        if ( _phiL < 0 )
            _phiL += 1;
        if ( _phiL > 1 )
            _phiL -= 1;
        if ( _phiR < 0 )
            _phiR += 1;
        if ( _phiR > 1 )
            _phiR -= 1;

        //if phase diff < 0.05 && magnitude diff < 0.005 && frequency > 700
        if ( ( abs( _phiL - _phiR ) < 0.05 && abs( _magL - _magR ) < 0.005 ) && _frq > 700 )
        {
            _specBufL[ i ] *= 0.1f;
            _specBufL[ i + 1 ] *= 0.1f;
            _specBufR[ i ] *= 0.1f;
            _specBufR[ i + 1 ] *= 0.1f;
        }
    }
}

//-------------------------------------------------------------------------------------------------

void DspStft::_IfftSpectralBuffers()
{
    ifft_kiss( &_specBufL[ 0 ], &_sigBufL[ 0 ], _sigBufL.size() );
    ifft_kiss( &_specBufR[ 0 ], &_sigBufR[ 0 ], _sigBufR.size() );
}

//=================================================================================================