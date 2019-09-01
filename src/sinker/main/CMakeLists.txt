set(COMPONENT_SRCS "bt_a2d.cpp"
                   "bt_avrcp.cpp"
					"bt_gap.cpp"
					"bt_iface.cpp"
					"bt_impl.cpp"
					"i2s_iface.cpp"
                   "main.cpp")
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(CMAKE_CXX_STANDARD 14)

register_component()
