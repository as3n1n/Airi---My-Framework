path = r"E:\SteamLibrary\steamapps\common\Phasmophobia\GameAssembly.dll"
offset = 0x1A67EF0
length = 256
with open(path,'rb') as f:
    f.seek(offset)
    data = f.read(length)
print('hex', data.hex())
print('ascii', ''.join(chr(b) if 32 <= b < 127 else '.' for b in data))
