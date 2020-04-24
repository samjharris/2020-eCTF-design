proc generate {drv_handle} {
        puts "Generate AXIS_AES_CTR"
        xdefine_include_file $drv_handle "xparameters.h" "AXIS_AES_CTR" "NUM_INSTANCES" "DEVICE_ID"  "key_size" "C_AXI_CTRL_BASEADDR" "C_AXI_CTRL_HIGHADDR"
}
