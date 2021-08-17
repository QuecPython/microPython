import uzlib as zlib

class UnZipUtils(object):
    def __init__(self, dest_dir="/usr/"):
        self.start_byte = b"|start"
        self.b_file_name = b"|filename|"
        self.b_file_size = b"|filesize|"
        self.b_file_content = b"|filecontent|"
        self.end_byte = b"|end|"
        self.index = len(self.start_byte)
        self.total_size = 0
        self.split_sign = b"|"
        self.file_name = ""
        self.file_name_list = []
        self.file_count = 0
        self.file_content = b""
        self.data = b""
        self.updater_dir = dest_dir

    def set_data(self, file_name):
        with open(self.updater_dir + file_name, "rb") as f:
            self.data = zlib.decompress(f.read())
            self.total_size = len(self.data)

    def get_file_name(self, offset):
        if self.data[offset:offset + 10] == self.b_file_size:
            return offset
        else:
            return self.get_file_name(offset + 1)

    def get_file_size(self, offset):
        if self.data[offset:offset + 13] == self.b_file_content:
            return offset
        else:
            return self.get_file_size(offset + 1)

    def run(self):
        # 获取file_name
        self.index = self.index + len(self.b_file_name)
        offset = self.get_file_name(self.index)
        self.file_name = self.data[self.index:offset].decode()
        # 获取file_size
        self.index = offset + len(self.b_file_size)
        offset = self.get_file_size(self.index)
        self.file_size = int(self.data[self.index:offset].decode())
        # 获取file_content
        self.index = offset + len(self.b_file_content)
        end_size = self.index + self.file_size
        self.file_content = self.data[self.index:end_size]
        self.save_file()
        self.index = end_size + len(self.end_byte)
        if self.data[end_size:self.index] == self.end_byte:
            self.file_name_list.append(self.file_name)
            if self.index == len(self.data):
                return 1
            else:
                self.index = self.index + len(self.start_byte)
                return self.run()
        else:
            return 0

    def save_file(self):
        with open(self.updater_dir + self.file_name, "wb") as f:
            f.write(self.file_content)

if __name__ == '__main__':
    tl = UnZipUtils("11")
    print(tl.split_sign[0])
    tl.set_data("main.py")
    code = tl.run()
    print(code)
