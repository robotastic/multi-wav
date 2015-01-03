
#include "logging_receiver_dsd.h"
using namespace std;

bool log_dsd::logging = false;

log_dsd_sptr make_log_dsd(float freq, float center, long s, long t, int n)
{
    return gnuradio::get_initial_sptr(new log_dsd(freq, center, s, t, n));
}
unsigned GCD(unsigned u, unsigned v) {
    while ( v != 0) {
        unsigned r = u % v;
        u = v;
        v = r;
    }
    return u;
}

std::vector<float> design_filter(double interpolation, double deci) {
    float beta = 5.0;
    float trans_width = 0.5 - 0.4;
    float mid_transition_band = 0.5 - trans_width/2;

    std::vector<float> result = gr::filter::firdes::low_pass(
                                                             interpolation,
                                                             1,
                                                             mid_transition_band/interpolation,
                                                             trans_width/interpolation,
                                                             gr::filter::firdes::WIN_KAISER,
                                                             beta
                                                             );

    return result;
}

log_dsd::log_dsd(float f, float c, long s, long t, int n)
    : gr::hier_block2 ("log_dsd",
          gr::io_signature::make  (1, 1, sizeof(gr_complex)),
          gr::io_signature::make  (0, 0, sizeof(float)))
{
	freq = f;
	center = c;
  samp_rate = s;
	talkgroup = t;
	num = n;
	active = false;

	timestamp = time(NULL);
	starttime = time(NULL);

	float offset = (f*1000000) - center; //have to flip for 3.7

	int samp_per_sym = 10;
	double decim = 80;
	float xlate_bandwidth = 14000; //24260.0;
	float channel_rate = 4800 * samp_per_sym;
	double pre_channel_rate = samp_rate/decim;



    lpf_taps =  gr::filter::firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 6000);
	//lpf_taps =  gr::filter::firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 3000);
	
	prefilter = gr::filter::freq_xlating_fir_filter_ccf::make(decim,
						      lpf_taps,
						       offset,
						       samp_rate);
	unsigned int d = GCD(channel_rate, pre_channel_rate);
    	channel_rate = floor(channel_rate  / d);
    	pre_channel_rate = floor(pre_channel_rate / d);
	resampler_taps = design_filter(channel_rate, pre_channel_rate);

	downsample_sig = gr::filter::rational_resampler_base_ccf::make(channel_rate, pre_channel_rate, resampler_taps);
	demod = gr::analog::quadrature_demod_cf::make(1.6); //1.4);
	levels = gr::blocks::multiply_const_ff::make(1.0); //33);

	for (int i=0; i < samp_per_sym; i++) {
		sym_taps.push_back(1.0 / samp_per_sym);
	}
	sym_filter = gr::filter::fir_filter_fff::make(1, sym_taps);
	/*
	if (!logging) {
	iam_logging = true;
	logging = true;
	std::cout << "I am the one true only!" << std::endl;
	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_C4FM,3,1,1, false, num);
	} else {
	iam_logging = false;
	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_C4FM,3,0,0, false, num);
	}*/
	
	iam_logging = false;
	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_C4FM,3, false, 0);

	tm *ltm = localtime(&starttime);

	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,timestamp,freq);
	sprintf(status_filename, "%s/%ld-%ld_%g.json", path_stream.str().c_str(),talkgroup,timestamp,freq);
	wav_sink = gr::blocks::wavfile_sink::make(filename,1,8000,16);
	null_sink = gr::blocks::null_sink::make(sizeof(gr_complex));

	sprintf(raw_filename, "%s/%ld-%ld_%g.raw", path_stream.str().c_str(),talkgroup,timestamp,freq);
	raw_sink = gr::blocks::file_sink::make(sizeof(gr_complex), raw_filename);


	connect(self(),0, null_sink,0);
}

log_dsd::~log_dsd() {

}
// from: /gnuradio/grc/grc_gnuradio/blks2/selector.py
void log_dsd::unmute() {
	// this function gets called everytime their is a TG continuation command. This keeps the timestamp updated.
	std::cout<< "logging_receiver_dsd.cc: Refreshing Logger [ " << num << " ] - Elapsed[ " << time(NULL) - timestamp << "]  " << std::endl; 

	timestamp = time(NULL);

}

void log_dsd::mute() {


}

bool log_dsd::is_active() {
	return active;
}

long log_dsd::get_talkgroup() {
	return talkgroup;
}

float log_dsd::get_freq() {
	return freq;
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


void log_dsd::tune_offset(float f) {
	freq = f;
	int offset_amount = ((f*1000000) - center);
	prefilter->set_center_freq(offset_amount); // have to flip this for 3.7
	//std::cout << "Offset set to: " << offset_amount << " Freq: "  << freq << std::endl;
}
void log_dsd::deactivate() {
	std::cout<< "logging_receiver_dsd.cc: Deactivating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ] " << std::endl; 

	active = false;

	//if (logging) {

  lock();

	wav_sink->close();
	
		raw_sink->close();
		disconnect(prefilter,0, raw_sink,0);

	
	disconnect(self(), 0, prefilter, 0);
	connect(self(),0, null_sink,0);




	disconnect(prefilter, 0, downsample_sig, 0);
	disconnect(downsample_sig, 0, demod, 0);
	
	disconnect(demod, 0, sym_filter, 0);
	disconnect(sym_filter, 0, levels, 0);
	disconnect(levels, 0, dsd, 0);
	disconnect(dsd, 0, wav_sink,0);
	

	unlock();

  //dsd_state *state = dsd->get_state();
  /*ofstream myfile (status_filename);
  if (myfile.is_open())
  {
    int level = (int) state->max / 164;
    int index=0;
    myfile << "{\n";
    myfile << "\"freq\": " << freq << ",\n";
    myfile << "\"num\": " << num << ",\n";
    myfile << "\"talkgroup\": " << talkgroup << ",\n";
    myfile << "\"center\": " << state->center << ",\n";
    myfile << "\"umid\": " << state->umid << ",\n";
    myfile << "\"lmid\": " << state->lmid << ",\n";
    myfile << "\"max\": " << state->max << ",\n";
    myfile << "\"inlvl\": " << level << ",\n";
    myfile << "\"nac\": " << state->nac << ",\n";
    myfile << "\"src\": " << state->lastsrc << ",\n";
    myfile << "\"dsdtg\": " << state->lasttg << ",\n";
    myfile << "\"headerCriticalErrors\": " << state->debug_header_critical_errors << ",\n";
    myfile << "\"headerErrors\": " << state->debug_header_errors << ",\n";
    myfile << "\"audioErrors\": " << state->debug_audio_errors << ",\n";
    myfile << "\"symbCount\": " << state->symbolcnt << ",\n";
    myfile << "\"srcList\": [ ";
    	while(state->src_list[index]!=0){
    		if (index !=0) {
    			 myfile << ", " << state->src_list[index];
    		} else {
    			myfile << state->src_list[index];
    		}
    		index++;
    	}
    myfile << " ]\n";
    myfile << "}\n";
    myfile.close();
  }
  else cout << "Unable to open file";
  dsd->reset_state();*/
//}
}

void log_dsd::activate(float f, int t, int n) {

	timestamp = time(NULL);
	starttime = time(NULL);

	talkgroup = t;
	freq = f;
  //num = n;
  	tm *ltm = localtime(&starttime);
  	std::cout<< "logging_receiver_dsd.cc: Activating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]  "  <<std::endl;

  	//if (logging) {
	prefilter->set_center_freq( (f*1000000) - center); // have to flip for 3.7

	if (iam_logging) {
		//printf("Recording Freq: %f \n", f);
	}



	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,timestamp,f);
	sprintf(status_filename, "%s/%ld-%ld_%g.json", path_stream.str().c_str(),talkgroup,timestamp,freq);
	
	sprintf(raw_filename, "%s/%ld-%ld_%g.raw", path_stream.str().c_str(),talkgroup,timestamp,freq);
    
	
	lock();

	raw_sink->open(raw_filename);
	wav_sink->open(filename);


	disconnect(self(),0, null_sink, 0);
	connect(self(),0, prefilter,0);
	connect(prefilter, 0, downsample_sig, 0);

	connect(prefilter,0, raw_sink,0);

	connect(downsample_sig, 0, demod, 0);
	connect(demod, 0, sym_filter, 0);
	connect(sym_filter, 0, levels, 0);
	connect(levels, 0, dsd, 0);
	connect(dsd, 0, wav_sink,0);
	
	unlock();
	//}
	active = true;

}
