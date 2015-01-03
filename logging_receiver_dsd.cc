
#include "logging_receiver_dsd.h"
using namespace std;

bool log_dsd::logging = false;

log_dsd_sptr make_log_dsd(long s,  int n)
{
    return gnuradio::get_initial_sptr(new log_dsd(s, n));
}


log_dsd::log_dsd( long s,  int n)
    : gr::hier_block2 ("log_dsd",
          gr::io_signature::make  (1, 1, sizeof(float)),
          gr::io_signature::make  (0, 0, sizeof(float)))
{
	samp_rate = s;
	num = n;
	active = false;

	timestamp = time(NULL);
	starttime = time(NULL);


	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/recordings";

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%d-0.wav", path_stream.str().c_str(),num);
	wav_sink = gr::blocks::wavfile_sink::make(filename,1,samp_rate,16);
	null_sink = gr::blocks::null_sink::make(sizeof(float));

	sprintf(raw_filename, "%s/%d-0.raw", path_stream.str().c_str(),num);
	raw_sink = gr::blocks::file_sink::make(sizeof(float), raw_filename);

	connect(self(),0, null_sink,0);
}

log_dsd::~log_dsd() {

}
// from: /gnuradio/grc/grc_gnuradio/blks2/selector.py

bool log_dsd::is_active() {
	return active;
}


char *log_dsd::get_filename() {
	return filename;
}

int log_dsd::lastupdate() {
	return time(NULL) - timestamp;
}

long log_dsd::elapsed() {
	return time(NULL) - starttime;
}


void log_dsd::deactivate() {
	std::cout<< "logging_receiver_dsd.cc: Deactivating Logger [ " << num << " ] " << std::endl; 

	active = false;


  lock();

	wav_sink->close();
	raw_sink->close();

	

	disconnect(self(),0, wav_sink,0);
	disconnect(self(),0, raw_sink,0);
	connect(self(),0, null_sink,0);


	unlock();

  
}

void log_dsd::activate(int n) {

	timestamp = time(NULL);
	starttime = time(NULL);


  	std::cout<< "logging_receiver_dsd.cc: Activating Logger [ " << num << " ]  "  <<std::endl;

	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/recordings";

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%d-%d.wav", path_stream.str().c_str(),num,n);
	
	sprintf(raw_filename, "%s/%d-%d.raw", path_stream.str().c_str(),num,n);
 	lock();

	raw_sink->open(raw_filename);
	wav_sink->open(filename);


	disconnect(self(),0, null_sink, 0);
	connect(self(),0, wav_sink,0);
	connect(self(),0, raw_sink,0);

	
	unlock();
	active = true;

}
