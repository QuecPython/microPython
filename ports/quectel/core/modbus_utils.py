from machine import UART
import ubinascii as binascii
import utime as time


class ModbusUtils:

    def __init__(self, uartN, baudrate, databits, parity, stopbit, flowctl):
        self.uart = UART(uartN, baudrate, databits, parity, stopbit, flowctl)

    @staticmethod
    def divmod_low_high(addr):
        high, low = divmod(addr, 0x100)
        return high, low

    def calc_crc(self, string_byte):
        crc = 0xFFFF
        for pos in string_byte:
            crc ^= pos
            for i in range(8):
                if ((crc & 1) != 0):
                    crc >>= 1
                    crc ^= 0xA001
                else:
                    crc >>= 1
        gen_crc = hex(((crc & 0xff) << 8) + (crc >> 8))
        int_crc = int(gen_crc, 16)
        return self.divmod_low_high(int_crc)

    def split_return_bytes(self, ret_bytes):
        ret_str = binascii.hexlify(ret_bytes, ',')
        return ret_str.split(b",")

    def read_uart(self):
        num = self.uart.any()
        msg = self.uart.read(num)
        ret_str = binascii.hexlify(msg, ',')
        return ret_str

    def write_coils(self, slave, const, start, coil_qty, crc_flag=True):
        start_h, start_l = self.divmod_low_high(start)
        coil_qty_h, coil_qty_l = self.divmod_low_high(coil_qty)
        data = bytearray([slave, const, start_h, start_l, coil_qty_h, coil_qty_l])
        print(data)
        if crc_flag:
            crc = self.calc_crc(data)
            for num in crc:
                data.append(num)
        self.uart.write(data)
        time.sleep_ms(1000)
        return True

    def write_coils_any(self, *args, crc_flag=True):
        data = bytearray(args)
        print(data)
        if crc_flag:
            crc = self.calc_crc(data)
            for num in crc:
                data.append(num)
        self.uart.write(data)
        time.sleep_ms(1000)
        return True

