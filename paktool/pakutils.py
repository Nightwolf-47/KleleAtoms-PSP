'''
PAKUtils - Classes to manage Quake PAK files

Made by Nightwolf-47
'''

import struct
import pathlib


class PakEntry:
    def __init__(self, name: str, data: bytes):
        self.name = name
        self.data = data
    
    def __str__(self):
        return f'Entry [path = "{self.name}", size = {len(self.data)}]'


class PakClass:
    def __init__(self, entries: list[PakEntry] | None = None):
        self.entries = entries or []

    def __str__(self):
        entry_str = ',\n'.join([str(e) for e in self.entries])
        return f'PakFile: {{\n{entry_str}\n}}'
    
    def readData(self, data: bytes):
        '''Reads Quake PAK data from a bytes object'''
        if not data:
            raise ValueError("PAK Data is not valid")
        elif len(data) < 12:
            raise ValueError(f"Size of data isn't big enough to fit the PAK header ({len(data)} < 12)")

        pak_size = len(data)

        # parse header
        header = struct.unpack_from("<4sII",data,0)
        if len(header) < 3:
            raise ValueError("Couldn't parse the PAK header")
        elif header[0] != b'PACK':
            pakstr = header[0].decode('utf-8',errors='replace')
            raise ValueError(f"PAK data is invalid ('{pakstr}' != 'PACK')")
        dir_offset = header[1]
        dir_size = header[2]
        entries_count = header[2] // 64
        
        if dir_offset+dir_size > pak_size:
            raise IndexError(f"PAK directory out of range! ({dir_offset}+{dir_size} > {pak_size})")
        
        #parse entries
        entries = []
        ent_struct = struct.Struct("<56sII")
        for i in range(entries_count):
            cur_pos = dir_offset + (i*64)
            entry_info = ent_struct.unpack_from(data,cur_pos)
            if len(entry_info) < 3:
                raise ValueError(f"Couldn't parse the PAK entry {i}")
            entry_name = entry_info[0].split(b'\0',1)[0].decode('utf-8')
            entry_offset = entry_info[1]
            entry_size = entry_info[2]
            entry_data = data[entry_offset:entry_offset+entry_size]
            entries.append(PakEntry(entry_name,entry_data))
        
        self.entries = entries


    def readFile(self, filePath: str):
        '''Reads a Quake PAK file'''
        data = None
        with open(filePath, "rb") as f:
            data = f.read()
        
        if data:
            self.readData(data)
        else:
            raise ValueError("PAK file couldn't be read")
        

    def writeData(self) -> bytes:
        '''Writes the contents of the entries list to an in-memory representation of Quake PAK file data'''
        ent_struct = struct.Struct("<56sii")
        dir_data = bytearray()
        entry_data = bytearray()
        data_pos = 12
        for e in self.entries:
            name_bytes = e.name.encode('utf-8')[:55] + b'\0'
            entry_size = len(e.data)
            entry_bytes = ent_struct.pack(name_bytes, data_pos, entry_size)
            dir_data.extend(entry_bytes)
            entry_data.extend(e.data)
            data_pos += entry_size

        if data_pos > 2147483647:
            raise ValueError(f"Directory offset position (total file size + 12) is too far for PAK format ({data_pos} > 2147483647)")

        header = struct.pack("<4sii",b'PACK',data_pos,len(self.entries)*64)
        return bytes(header + entry_data + dir_data)


    def writeFile(self, filePath: str):
        '''Writes the contents of the entries list to a PAK file'''
        data = self.writeData()

        with open(filePath, "wb") as f:
            f.write(data)


    def extract(self, output_path: str):
        '''
        Extract the PAK class entries to a given folder.  
        
        The output folder should be empty, otherwise some entries may not extract properly.  
        In case of duplicate entries, only the first occurence will be extracted.
        '''
        path = pathlib.Path(output_path)

        for e in self.entries:
            entry_path = path / e.name
            if entry_path.exists():
                continue
            entry_path.parent.mkdir(mode=0o770, parents=True, exist_ok=True)
            with entry_path.open('wb') as f:
                f.write(e.data)


    def import_entries(self, folder_path: str):
        '''
        Import entries from a given folder by recursively including all files inside of it.  
        
        The folder will be the root of the PAK file (all PAK entry paths are relative to it).  
        The PAK format has a limit of 55 characters for file/entry paths, so they must not be too long or complex.
        '''
        path = pathlib.Path(folder_path)
        if not path.is_dir():
            raise ValueError(f"Folder path '{folder_path}' does not point to a valid folder!")

        entries = []
        for p in path.rglob('*'):
            if p.is_file():
                file_name = p.relative_to(path).as_posix()
                if len(file_name) > 55:
                    raise ValueError(f"File path {file_name} is longer than 55 characters!")
                file_data = None
                with p.open('rb') as f:
                    file_data = f.read()
                if not file_data:
                    raise ValueError(f"Couldn't read the file {file_name}")
                entries.append(PakEntry(file_name,file_data))

        self.entries.extend(entries)
