import sys, os, argparse, json
from struct import unpack, calcsize
from glob import glob

# Parse the entire header
# https://github.com/FalconChristmas/fpp/blob/6a32eeb8d0630a37502725fb8f51cf686f15b28d/docs/FSEQ_Sequence_File_Format.txt
def parse_header(filename):
  header = {}
  format_magic  = '<4s'
  format_common = '<HBBHLLBB'
  format_v1     = '<LLBBL'
  format_v2     = '<BBBBQ'

  with open(filename, mode='rb') as file:
    header['file'] = os.path.basename(filename)

    # Check magic first
    data = file.read(calcsize(format_magic))
    header['magic'], *_ = unpack(format_magic, data)
    if (header['magic'] != b'PSEQ'):
      return
    header['magic'] = header['magic'].decode('utf-8')

    # Common Header
    data = file.read(calcsize(format_common))
    ( header['offset'],
      header['ver_min'],
      header['ver_maj'],
      header['length'],
      header['channels'],
      header['frames'],
      header['step'],
      header['flags']
    ) = unpack(format_common, data)

    # V1 Header
    if (header['ver_maj'] == 1):
      data = file.read(calcsize(format_v1))
      ( header['uni_count'],
        header['uni_size'],
        header['gamma'],
        header['color'],
        header['reserved']
      ) = unpack(format_v1, data)

    # V2 Header
    elif (header['ver_maj'] == 2):
      data = file.read(calcsize(format_v2))
      ( header['compression'],
        header['num_comp'],
        header['num_sparse'],
        header['flags2'],
        header['uid']
      ) = unpack(format_v2, data)

      # V2.1 12bit compression blocks
      header['num_comp'] = (header['compression'] & 0xF0) | header['num_comp']
      header['compression'] = header['compression'] & 0xF


    # Variable Header(s)
    file.seek(header['length'])
    while file.tell() < header['offset']:
      len, *_ = unpack('<H', file.read(2))
      type, *_ = unpack('<2s', file.read(2))
      len = len - 4

      if type == b'mf':
        header['mf'], *_ = unpack(f'<{len}s', file.read(len))
        header['mf'] = header['mf'][:-1].decode('utf-8')
      elif type == b'sp':
        header['sp'], *_ = unpack(f'<{len}s', file.read(len))
        header['sp'] = header['sp'][:-1].decode('utf-8')

    return header

# Simple Print (default)
def print_simple(h):
  sparse = ""
  compressed = ""
  if h['ver_maj'] == 2:
    ctype = 'unknown'
    if h['compression'] == 1:
      ctype = 'zstd'
    elif h['compression'] == 2:
      ctype = 'zlib'
    sparse = " sparse" if h['num_sparse'] else ""
    compressed = f" compressed ({ctype})" if h['compression'] else " uncompressed"

  sp = f" from {h['sp']}" if "sp" in h else ""

  print(f"{h['file']}: V{h['ver_maj']}.{h['ver_min']}{sparse}{compressed} "
        f"{h['channels']} channels with {h['frames']} frames @ {h['step']}ms{sp}")

# Verbose Print
def print_verbose(h):
  print(f"\n{h['file']}")
  print(''.join(['-' * len(h['file'])]))
  print(f"File ID:        {h['magic']}")
  print(f"Data Offset:    {h['offset']}")
  print(f"Minor Version:  {h['ver_min']}")
  print(f"Major Version:  {h['ver_maj']}")
  print(f"Header Length:  {h['length']}")
  print(f"Channel Count:  {h['channels']}")
  print(f"Frame Count:    {h['frames']}")
  print(f"Step Time(ms):  {h['step']}")
  print(f"Flags:          {h['flags']}")

  if h['ver_maj'] == 1:
    print(f"Universe Count: {h['uni_count']}")
    print(f"Universr Size:  {h['uni_size']}")
    print(f"Gamma:          {h['gamma']}")
    print(f"Color:          {h['color']}")
    print(f"Reserved:       {h['reserved']}")

  elif h['ver_maj'] == 2:
    print(f"Compression:    {h['compression']}")
    print(f"Comp Blocks:    {h['num_comp']}")
    print(f"Sparse Ranges:  {h['num_sparse']}")
    print(f"FLags 2:        {h['flags2']}")
    print(f"Unique ID:      {h['uid']}")

  if 'mf' in h:
    print(f"Media File:     {h['mf']}")

  if 'sp' in h:
    print(f"Created By:     {h['sp']}")

# main
parser = argparse.ArgumentParser(
    description = 'Dump FSEQ file information.'
)
pgroup = parser.add_mutually_exclusive_group()
pgroup.add_argument('-v', '--verbose', help = "Show all header information", action = "store_true")
pgroup.add_argument('-j', '--json', help = "Dump as JSON", action = "store_true")
pgroup.add_argument('-d', '--dict', help = "Dump as python dictionary", action = "store_true")
parser.add_argument('files', nargs = '+')
args = parser.parse_args()

if not args.files:
  parser.print_help()

files = []
for file in args.files:
  files += glob(file)

for file in files:
  try:
    header = parse_header(file)
    if header is None:
      print (f"\n*** {file} is not a FSEQ file ***\n")
      continue
    if args.verbose:
      print_verbose(header)
    elif args.dict:
      print(header)
    elif args.json:
      print(json.dumps(header))
    else:
      print_simple(header)
  except (FileNotFoundError, IsADirectoryError) as err:
    print (f"{sys.argv[0]}: {file}: {err.strerror}", file=sys.stderr)
