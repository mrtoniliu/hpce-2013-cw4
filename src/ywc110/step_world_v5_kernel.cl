enum cell_flags_t{
    Cell_Fixed      =0x1,
    Cell_Insulator  =0x2
};

__kernel void kernel_xy(
	float alpha,
	float dt,
	__global const float *world_state,
	__global  const uint *world_properties,
	__global float *buffer) {

	uint x=get_global_id(0);
    uint y=get_global_id(1);
    uint w=get_global_size(0);

	float outer=alpha*dt;		// We spread alpha to other cells per time
	float inner=1-outer/4;				// Anything that doesn't spread stays
	unsigned index=y*w + x;

	uint properties = world_properties[index];

	if((properties & Cell_Fixed) || (properties & Cell_Insulator)){
		// Do nothing, this cell never changes (e.g. a boundary, or an interior fixed-value heat-source)
		buffer[index]=world_state[index];
	}else{
		float contrib=inner;
		float acc=inner*world_state[index];

		// Cell above
		if(properties & 0x4) { // index - w
			contrib += outer;
			acc += outer * world_state[index-w];
		}

		// Cell below
		if(properties & 0x8) { // index + w
			contrib += outer;
			acc += outer * world_state[index+w];
		}

		// Cell left
		if(properties & 0x10) { // index - 1
			contrib += outer;
			acc += outer * world_state[index-1];
		}

		// Cell right
		if(properties & 0x20) { // index + 1
			contrib += outer;
			acc += outer * world_state[index+1];
		}

		// Scale the accumulate value by the number of places contributing to it
		float res=acc/contrib;
		// Then clamp to the range [0,1]
		res= min(1.0f, max(0.0f, res));
		buffer[index] = res;

	}
}