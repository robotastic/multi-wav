
#include "recorder.h"
using namespace std;

bool recorder::logging = false;

recorder_sptr make_recorder(long s,  int n)
{
    return gnuradio::get_initial_sptr(new recorder(s, n));
}


recorder::recorder( long s,  int n)
    : gr::hier_block2 ("recorder",
          gr::io_signature::make  (1, 1, sizeof(float)),
          gr::io_signature::make  (0, 0, sizeof(float)))
{
	samp_rate = s;
	num = n;
	active = true;

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

	connect(self(),0, wav_sink,0);
	connect(self(),0, raw_sink,0);
}

recorder::~recorder() {

}

bool recorder::is_active() {
	return active;
}


char *recorder::get_filename() {
	return filename;
}

int recorder::lastupdate() {
	return time(NULL) - timestamp;
}

long recorder::elapsed() {
	return time(NULL) - starttime;
}


void recorder::deactivate() {
	std::cout<< "Deactivating Logger [ " << num << " ] " << std::endl; 

	active = false;


  lock();

	wav_sink->close();
	raw_sink->close();

	disconnect(self(),0, wav_sink,0);
	disconnect(self(),0, raw_sink,0);
	connect(self(),0, null_sink,0);

	unlock();
}


void recorder::activate(int n) {

	timestamp = time(NULL);
	starttime = time(NULL);


  	std::cout<< "Activating Logger [ " << num << " ]  "  <<std::endl;
  	active = true;

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
}
