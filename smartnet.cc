/*
 * Copyright 2011 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */


/*
 * GNU Radio C++ example creating dial tone
 * ("the simplest thing that could possibly work")
 *
 * Send a tone each to the left and right channels of stereo audio
 * output and let the user's brain sum them.
 *
 * GNU Radio makes extensive use of Boost shared pointers.  Signal processing
 * blocks are typically created by calling a "make" factory function, which
 * returns an instance of the block as a typedef'd shared pointer that can
 * be used in any way a regular pointer can.  Shared pointers created this way
 * keep track of their memory and free it at the right time, so the user
 * doesn't need to worry about it (really).
 *
 */

// Include header files for each block used in flowgraph

#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>



#include <boost/program_options.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/tokenizer.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <gnuradio/msg_queue.h>
#include <gnuradio/message.h>
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/gr_complex.h>

#include <gnuradio/top_block.h>
#include <gnuradio/blocks/multiply_cc.h>

#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_ccf.h>

#include <gnuradio/digital/fll_band_edge_cc.h>
#include <gnuradio/digital/clock_recovery_mm_ff.h>
#include <gnuradio/digital/binary_slicer_fb.h>
#include <gnuradio/digital/correlate_access_code_tag_bb.h>



#include <gnuradio/analog/pll_freqdet_cf.h>
#include <gnuradio/analog/sig_source_f.h>
#include <gnuradio/analog/sig_source_c.h>

#include <gnuradio/blocks/vector_source_f.h>
 #include <gnuradio/blocks/null_sink.h>
 #include <gnuradio/blocks/throttle.h>
 #include <cstdlib>
 #include <ctime>
#include "logging_receiver_dsd.h"


 namespace po = boost::program_options;

 using namespace std;

 int thread_num=0;
bool console  = false;




 time_t last_monkey;
 vector<log_dsd_sptr> loggers;
 unsigned int max_loggers = 6;
 unsigned int num_loggers = 0;
 unsigned int recording_num = 0;
 vector<log_dsd_sptr> active_loggers;

gr::top_block_sptr tb;
gr::blocks::vector_source_f::sptr src;
gr::blocks::throttle::sptr throttle;
gr::blocks::null_sink::sptr null_sink;



 volatile sig_atomic_t exit_flag = 0;






void exit_interupt(int sig){ // can be called asynchronously
  exit_flag = 1; // set flag
}


void init_loggers(long samp_rate) {

// static loggers
	for (int i = 0; i < max_loggers; i++) {
		log_dsd_sptr log = make_log_dsd( samp_rate, i);

		loggers.push_back(log);
		tb->connect(throttle, 0, log, 0);
		std::cout << "Created and connect logger: " << i << " address: " << log << std::endl;
	}

}

void start_loggers() {
for(vector<log_dsd_sptr>::iterator it = loggers.begin(); it != loggers.end();it++) {
				log_dsd_sptr rx = *it;
				if (!rx->is_active())
				{
					num_loggers++;
					recording_num++;
					rx->activate(recording_num);
				}
			}

}

void monkey(){ //const boost::system::error_code&) {
	int logger_num = rand() % max_loggers;

	if (loggers[logger_num]->is_active()) {
		loggers[logger_num]->deactivate();
		num_loggers--;
	} else {
		num_loggers++;
		recording_num++;
		loggers[logger_num]->activate(recording_num);
	}
}

int main(int argc, char **argv)
{

	std::string device_addr;
	double  samp_rate=32000;
    //setup the program options


	signal(SIGINT, exit_interupt);


    tb = gr::make_top_block("Smartnet");

    vector<float> floatVector;
	srand((unsigned)time(NULL));
	
	for (int i =0; i < 2000; i++){
        float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        floatVector.push_back(r);
    }
    src = gr::blocks::vector_source_f::make(floatVector,true);
    throttle = gr::blocks::throttle::make(sizeof(float), samp_rate);
    null_sink = gr::blocks::null_sink::make(sizeof(float));


	tb->connect(src,0,throttle,0);
	tb->connect(throttle,0,null_sink,0);

	init_loggers(samp_rate);
	start_loggers();
	last_monkey = time(NULL);

	tb->start();

	/*
	boost::asio::io_service io;

  boost::asio::deadline_timer t(io, boost::posix_time::seconds(5));
  t.async_wait(&monkey);

  io.run();*/

	while (1) {
		if(exit_flag){ // my action when signal set it 1
			printf("\n Signal caught!\n");
			tb->stop();
			return 0;
		}
		if ((time(NULL) - last_monkey) > 10) {
			last_monkey = time(NULL);
			monkey();
		}
			
	}


  // Exit normally.
	return 0;
		
}
