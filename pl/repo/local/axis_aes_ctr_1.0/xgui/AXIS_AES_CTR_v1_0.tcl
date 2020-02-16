# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  set ctrl_data_width [ipgui::add_param $IPINST -name "ctrl_data_width" -widget comboBox]
  set_property tooltip {The width of the AXI-Lite control bus} ${ctrl_data_width}
  set stream_data_width [ipgui::add_param $IPINST -name "stream_data_width" -widget comboBox]
  set_property tooltip {The width of the AXI-Stream data bus} ${stream_data_width}
  set key_size [ipgui::add_param $IPINST -name "key_size" -widget comboBox]
  set_property tooltip {AES Key Size} ${key_size}

}

proc update_PARAM_VALUE.C_AXI_CTRL_BASEADDR { PARAM_VALUE.C_AXI_CTRL_BASEADDR } {
	# Procedure called to update C_AXI_CTRL_BASEADDR when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.C_AXI_CTRL_BASEADDR { PARAM_VALUE.C_AXI_CTRL_BASEADDR } {
	# Procedure called to validate C_AXI_CTRL_BASEADDR
	return true
}

proc update_PARAM_VALUE.C_AXI_CTRL_HIGHADDR { PARAM_VALUE.C_AXI_CTRL_HIGHADDR } {
	# Procedure called to update C_AXI_CTRL_HIGHADDR when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.C_AXI_CTRL_HIGHADDR { PARAM_VALUE.C_AXI_CTRL_HIGHADDR } {
	# Procedure called to validate C_AXI_CTRL_HIGHADDR
	return true
}

proc update_PARAM_VALUE.ctrl_data_width { PARAM_VALUE.ctrl_data_width } {
	# Procedure called to update ctrl_data_width when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.ctrl_data_width { PARAM_VALUE.ctrl_data_width } {
	# Procedure called to validate ctrl_data_width
	return true
}

proc update_PARAM_VALUE.key_size { PARAM_VALUE.key_size } {
	# Procedure called to update key_size when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.key_size { PARAM_VALUE.key_size } {
	# Procedure called to validate key_size
	return true
}

proc update_PARAM_VALUE.stream_data_width { PARAM_VALUE.stream_data_width } {
	# Procedure called to update stream_data_width when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.stream_data_width { PARAM_VALUE.stream_data_width } {
	# Procedure called to validate stream_data_width
	return true
}


proc update_MODELPARAM_VALUE.ctrl_data_width { MODELPARAM_VALUE.ctrl_data_width PARAM_VALUE.ctrl_data_width } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.ctrl_data_width}] ${MODELPARAM_VALUE.ctrl_data_width}
}

proc update_MODELPARAM_VALUE.stream_data_width { MODELPARAM_VALUE.stream_data_width PARAM_VALUE.stream_data_width } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.stream_data_width}] ${MODELPARAM_VALUE.stream_data_width}
}

proc update_MODELPARAM_VALUE.C_AXI_CTRL_BASEADDR { MODELPARAM_VALUE.C_AXI_CTRL_BASEADDR PARAM_VALUE.C_AXI_CTRL_BASEADDR } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.C_AXI_CTRL_BASEADDR}] ${MODELPARAM_VALUE.C_AXI_CTRL_BASEADDR}
}

proc update_MODELPARAM_VALUE.C_KEY_SIZE { MODELPARAM_VALUE.C_KEY_SIZE } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	# WARNING: There is no corresponding user parameter named "C_KEY_SIZE". Setting updated value from the model parameter.
set_property value 256 ${MODELPARAM_VALUE.C_KEY_SIZE}
}

