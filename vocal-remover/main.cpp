/************************************************************************
DSPatch - Cross-Platform, Object-Oriented, Flow-Based Programming Library
Copyright (c) 2012-2015 Marcus Tomlinson

This file is part of DSPatch.

GNU Lesser General Public License Usage
This file may be used under the terms of the GNU Lesser General Public
License version 3.0 as published by the Free Software Foundation and
appearing in the file LGPLv3.txt included in the packaging of this
file. Please review the following information to ensure the GNU Lesser
General Public License version 3.0 requirements will be met:
http://www.gnu.org/copyleft/lgpl.html.

Other Usage
Alternatively, this file may be used in accordance with the terms and
conditions contained in a signed written agreement between you and
Marcus Tomlinson.

DSPatch is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
************************************************************************/

#include <DSPatch.h>

#include <DspWaveStreamer.h>
#include <DspAudioDevice.h>
#include <DspStft.h>

#include <iostream>
#include <stdio.h>

//=================================================================================================
// This is a simple program that streams an wave out of an audio device,
// then removes centre phased signals (the vocals in music) when a key is pressed.

int main()
{
    std::string input;

    // 1. Stream wave
    // ==============

    // create a circuit
    DspCircuit circuit;

    // declare components to be added to the circuit
    DspWaveStreamer waveStreamer;
    DspStft stft;
    DspAudioDevice audioDevice;

    // set circuit thread count to 2
    circuit.SetThreadCount( 2 );

    // start separate thread to tick the circuit continuously ("auto-tick")
    circuit.StartAutoTick();

    // add new components to the circuit
    circuit.AddComponent( waveStreamer );
    circuit.AddComponent( stft );
    circuit.AddComponent( audioDevice );

    // DspWaveStreamer has an output signal named "Sample Rate" that streams the current wave's sample rate
    // DspAudioDevice's "Sample Rate" input receives a sample rate value and updates the audio stream accordingly
    circuit.ConnectOutToIn( waveStreamer, "Sample Rate", stft, "Sample Rate" );  // sample rate sync
    circuit.ConnectOutToIn( waveStreamer, "Sample Rate", audioDevice, "Sample Rate" );  // sample rate sync

    // load a wave into the wave streamer and start playing the track
    waveStreamer.LoadFile( "./Sample.wav" );
    waveStreamer.Play();

    while ( input != "q" )
    {
        // connect component output signals to respective component input signals
        circuit.ConnectOutToIn( waveStreamer, 0, audioDevice, 0 );  // wave left channel into audioDevice left
        circuit.ConnectOutToIn( waveStreamer, 1, audioDevice, 1 );  // wave right channel into audioDevice right

        // wait for key press
        std::cout << "Press ENTER to remove vocals ('q' + ENTER to quit)" << std::endl;
        getline(std::cin, input);

        if ( input != "q" )
        {
            // 2. Remove vocals
            // ================
            
            // connect component output signals to respective component input signals
            circuit.ConnectOutToIn( waveStreamer, 0, stft, 0 );  // wave left channel into stft left
            circuit.ConnectOutToIn( waveStreamer, 1, stft, 1 );  // wave right channel into stft right
            circuit.ConnectOutToIn( stft, 0, audioDevice, 0 );  // stft left channel into audioDevice left
            circuit.ConnectOutToIn( stft, 1, audioDevice, 1 );  // stft right channel into audioDevice right

            // wait for key press
            std::cout << "Press ENTER to re-add vocals ('q' + ENTER to quit)" << std::endl;
            getline(std::cin, input);
        }
    }

    // clean up DSPatch
    DSPatch::Finalize();

    return 0;
}

//=================================================================================================
