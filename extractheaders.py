import sys

def to_binary(string):
    ret = ''
    for char in string:
        b = ord(char)
        ret += '{0:08b}'.format(b)
    return ret

def hex_to_string(string):
    output = ''
    for i in range(0, len(string), 2):
        if (len(string[i:i+2]) > 1):
            output += (chr(int(string[i:i+2] ,16)))
    return output



total = 0
f = open(sys.argv[1])
out = []
for line in f:
    out.append(line)
    total += len(line)
    if total > 1000000:
        break

all_data = ''.join(out)
no_hex = hex_to_string(all_data)
data = to_binary(no_hex)
wat = to_binary('1AY&SY')
index = 0
ret = data.find(wat, index)
last_ret = -47
while ret >= 0:
    print ret
    last_ret = ret
    index = ret + 1
    ret = data.find(wat, index)

print 'OFFSET: ', (1348164 - last_ret ) % 8
print len(data), 1348164
extracted = data[last_ret:1348164]
print 'llen', len(extracted), len(extracted) % 8, len(extracted) / 8 , len(data[last_ret:])

extracted += to_binary(hex_to_string('177245385090'))
extracted += extracted[48:48+32]
hexx = ''
while len(extracted) % 8 != 0:
    extracted += '0'
    print 'add'

for i in range(0, len(extracted), 8):
    val = 0
    for j in range(8):
        val *= 2
        val += ord(extracted[i+j]) - ord('0')
    hexx += '{:02X}'.format(val)


package = '425A6839' + hexx 
with open('posledny_blok.bz2', 'w') as f:
    f.write(hex_to_string(package))
