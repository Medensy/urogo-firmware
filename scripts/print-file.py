import sys
import struct

if __name__ == '__main__':
  file_name = '2020-05-28-15-58-10.bin'
  if len(sys.argv) == 3 and sys.argv[1] == '-f':
    file_name = sys.argv[2]

  print('open file:', file_name)
  with open(file_name, 'rb') as f:
    data = f.read()

  data_length = int(len(data)/2) # int16 data length
  data = struct.unpack('h'*data_length, data)
  
  for i, x in enumerate(data):
    print('{}) {}'.format(i,x))
