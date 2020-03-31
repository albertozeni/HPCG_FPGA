#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <iostream>
#include "common.h"
//#include "kernel_stream.h"

using namespace hls;
using namespace std;

typedef qdma_axis<BLOCK, 0, 0, 0> pkt;


void execute(stream<pkt> &a, stream<pkt> &b, stream<pkt> &res, synt_type *alpha, synt_type *beta){

	execution_loop:while(true){
	#pragma HLS pipeline II=1
		// Reading a and b streaming into packets
		pkt a_tmp = a.read();
		pkt b_tmp = b.read();
		// Packet for output
		pkt res_tmp;

		// Reading data from input packet
		ap_uint<512> a_tmp_block = a_tmp.get_data();
		ap_uint<512> b_tmp_block = b_tmp.get_data();

		// Creating temp block for the result
		ap_uint<512> res_tmp_block;

		// Compute mult and vadd
		split_block_loop:for(unsigned int j = 0; j < BLOCK/TYPE; j++){
		#pragma HLS unroll
			ap_uint<TYPE> a_tmp_uint = a_tmp_block.range((j+1)*TYPE-1, j*TYPE);
			ap_uint<TYPE> b_tmp_uint = b_tmp_block.range((j+1)*TYPE-1, j*TYPE);
			synt_type a_val = *((synt_type *)&a_tmp_uint);
			synt_type b_val = *((synt_type *)&b_tmp_uint);
			synt_type res_val = (*alpha)*a_val+(*beta)*b_val;
			res_tmp_block.range((j+1)*TYPE-1, j*TYPE) = *(ap_uint<TYPE> *)&res_val;	
		}

		// Setting data and configuration to output packet
		res_tmp.set_data(res_tmp_block);
		res_tmp.set_last(a_tmp.get_last());
		res_tmp.set_keep(-1); // Enabling all bytes
		
		// Writing packet to output stream
		res.write(res_tmp);

		if (res_tmp.get_last()) {
            break;
        }
	}

}


extern "C"{
void vecsumprod(stream<pkt> &a, stream<pkt> &b, stream<pkt> &res, synt_type *alpha, synt_type *beta){
	#pragma HLS interface axis port=a  //bundle = gmem0
	#pragma HLS interface axis port=b  //bundle = gmem1
	#pragma HLS interface axis port=res //bundle = gmem2

	#pragma HLS interface s_axilite port = alpha bundle = control
	#pragma HLS interface s_axilite port = beta bundle = control
	#pragma HLS interface s_axilite port = return bundle = control
	#pragma HLS interface ap_ctrl_chain port = return

	#pragma HLS inline recursive
	#pragma HLS dataflow

	execute(a, b, res, alpha, beta);
	
}
}

