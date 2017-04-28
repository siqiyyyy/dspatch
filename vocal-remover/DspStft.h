#ifndef DSPSTFT_H
#define DSPSTFT_H

#include "DSPatch.h"

//=================================================================================================

class DspStft : public DspComponent
{
public:
  DspStft();
  virtual ~DspStft();

protected:
  virtual void Process_( DspSignalBus& inputs, DspSignalBus& outputs );

private:
  unsigned short _bufferSize;
  long _sampleRate;

  std::vector< float > _hanningLookup;

  std::vector< float > _inputBufL;
  std::vector< float > _inputBufR;
  std::vector< float > _prevInputBufL;
  std::vector< float > _prevInputBufR;
  std::vector< float > _outputBufL;
  std::vector< float > _outputBufR;

  std::vector< float > _sigBufL;
  std::vector< float > _sigBufR;
  std::vector< float > _specBufL;
  std::vector< float > _specBufR;

  std::vector< float > _mixBufL;
  std::vector< float > _mixBufR;

  void _SetBufferSize( unsigned short bufferSize );

  void _InitBuffers();
  void _BuildHanningLookup();

  void _BuildSignalBuffers();
  void _StoreCurrentBuffers();
  void _ApplyHanningWindows();
  void _FftSignalBuffers();
  void _ProcessSpectralBuffers();
  void _IfftSpectralBuffers();

  DspMutex _busyMutex;
};

//=================================================================================================

#endif // DSPSTFT_H
