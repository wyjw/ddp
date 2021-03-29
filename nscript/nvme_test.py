import subprocess
from optparse import OptionParser, OptionGroup
import pathlib

if __name__ == "__main__":
    parser = OptionParser("usage: [options] drive")

    parser.add_option("", "--insert", dest="ins", action="store_true", default=False)
    parser.add_option("", "--check", dest="chk", action="store_true", default=True)
    (opts, args) = parser.parse_args()

    filename = '/dev/nvme0n1'
    bs = 512
    nb = [970]
    ms = 470
   
    
    if opts.ins:
        for n in nb: 
            ilw = []
            ilw.append('sudo')
            ilw.append('nvme')
            ilw.append('write')
            ilw.append('--start-block')
            ilw.append(str(n))
            ilw.append('-z')
            ilw.append(str(bs))

            # make a file and write data to it
            fn = 'tmp1.txt'
            with open(fn, 'w') as f:
                f.write(str(ms))
                f.write(str(0) * (bs-1))
            ilw.append('-d')
            ilw.append(str(pathlib.Path(__file__).parent.absolute()) + '/' + fn)
            ilw.append(filename)
            print(ilw)
            try:
                subprocess.Popen(ilw, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                raise RuntimeError("command '{}', returned with error (code: {}): {}".format(e.cmd, e.returncode, e.output))

    if opts.chk:
        for n in nb:
            ilr = []
            ilr.append('sudo')
            ilr.append('nvme')
            ilr.append('read')
            ilr.append('--start-block')
            ilr.append(str(n))
            ilr.append('-z')
            ilr.append(str(bs))
            ilr.append(filename)
            print(ilr)
            p2 = subprocess.Popen(ilr, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            

    if opts.ins and opts.chk:
        ou = p2.stdout.read()[0:(bs)]
        with open(fn, 'rb') as f1:
            fou = f1.read()[0:(bs)]
        if ou == fou:
            print("No problem.")
        else:
            print("Problem between {} and {}", ou, fou)
    else:
        ou = p2.stdout.read()[0:bs]
        print('RESULT:', ou)
