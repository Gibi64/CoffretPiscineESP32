idf_component_register(
    SRCS
        "CoffretPiscine.cpp"
        "Protocole.cpp"
        "CProtocoleModbusRJE.cpp"
        "CProtocoleFaulhaber.cpp"
        "CProtocoleEurotherme.cpp"
        "CProtocoleJBus.cpp"
        "CProtocoleModbus.cpp"
        "CProtocoleAscii.cpp"
        "CProtocoleMatsu.cpp"
        "CProtocoleBaldor.cpp"
        "CProtocoleHbm.cpp"
        "CProtocoleAleatoire.cpp"
        "CSerialClient.cpp"
        "modbus_convert.cpp"
        "InitLog.cpp"
        "Entry.cpp"
        "ReadConfig.cpp"
        "CLaPoste.cpp"
    INCLUDE_DIRS "."
)

target_compile_definitions(${COMPONENT_TARGET} PRIVATE _ESP32)
target_compile_options(${COMPONENT_TARGET} PRIVATE -Wno-error)