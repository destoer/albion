#include <gb/apu.h>



namespace gameboy_psg
{


}


namespace gameboy
{

dtr_res Apu::load_state(std::ifstream &fp)
{
	dtr_res err = file_read_var(fp,down_sample_cnt);
	err |= psg.load_state(fp);

	return err;
}


void Apu::save_state(std::ofstream &fp)
{
	file_write_var(fp,down_sample_cnt);
	psg.save_state(fp);
}

}