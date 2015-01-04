#ifndef LPF_H
#define LPF_H

#include <cstdio>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <fstream>


#include <boost/shared_ptr.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <gnuradio/io_signature.h>
#include <gnuradio/hier_block2.h>

#include <gnuradio/block.h>
#include <gnuradio/blocks/null_sink.h>

#include <gnuradio/blocks/head.h>

#include <gnuradio/blocks/wavfile_sink.h>
#include <gnuradio/blocks/file_sink.h>



class recorder;

typedef boost::shared_ptr<recorder> recorder_sptr;

recorder_sptr make_recorder( long s,  int n);

class recorder : public gr::hier_block2
{
  friend recorder_sptr make_recorder( long s, int n);
protected:
    recorder(long s,  int n);

public:
    ~recorder();
    void lock_cycle();

private:
	
  long samp_rate;
	time_t timestamp;
	time_t starttime;
	char filename[160];
  char raw_filename[160];
	int num;


    /* GR blocks */

	gr::blocks::wavfile_sink::sptr wav_sink;
	gr::blocks::file_sink::sptr raw_sink;
	gr::blocks::null_sink::sptr null_sink;

};


#endif
