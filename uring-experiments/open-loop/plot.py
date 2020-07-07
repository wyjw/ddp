# pip3 install seaborn==0.9.0

import sys
import os
import os.path
import seaborn as sns
import pandas as pd

def get_data(pth=os.path.join('..', 'results', 'latest', 'output.data')):
    with open(pth) as f:
        headers = f.readlines(1)[0][:-1]
    header_list = headers.split(";")



    df = pd.read_csv(pth, sep=';',skiprows=[0],header=None)

    a = df.columns.size - len(header_list)
    if (not a):
        df.columns = header_list
    else:
        assert(a%9==0)
        disk_header = header_list[-9:]
        new_header = header_list + [h+str(i) for i in range(int(a/9)) for h in disk_header]
        df.columns = new_header

    df['99th']=[int(v.split('=')[1]) for v in df['read_clat_pct13']]
    df['50th']=[int(v.split('=')[1]) for v in df['read_clat_pct07']]

    # This rounding only adds 1% error on the x-axis and it helps
    # Seaborn realize that nearby points belong to the same cluster
    # even when effective throughput is varying somewhat.
    df['read_iops']=[int(round(float(v)/1000, 0)*1000) for v in df['read_iops']]
    df.astype({'read_iops': 'int32'}).dtypes
    df['jobs'] = df['jobs'].astype('category')
    return df

def plot(df=None, col='99th'):
    if df is None:
        df = get_data()
    sns.set_context('paper')
    sns.set_palette(sns.color_palette('Set1'))
    p = sns.relplot(data=df,
                    x='read_iops',
                    y=col,
                    hue='jobs',
                    style='polling',
                    kind='line',
                    col='engine',
                    row='dev',
                    ci=None,
                    markers=True)
    l = '99th Percentile Latency (us)'
    if col == '50th':
        l = 'Median Latency (us)'
    p = p.set_axis_labels('Throughput (IOPS)', l)
    p = p.set(xlim=(0, 700e3),
              ylim=(0, 100),
              yticks=[i*10 for i in range(0, 11)])
    p.fig.set_size_inches(12, 3.5)
    return p


def main():
    if len(sys.argv) < 2:
        print('Usage: %s <resultslogdir>' % sys.argv[0])
        raise SystemExit()

    logdir = sys.argv[1]
    df = get_data(os.path.join(logdir, 'output.data'))
             
    p = plot(df, '99th')
    p.savefig(os.path.join(logdir, 'plot-99th.png'), dpi=600)
    p.savefig(os.path.join(logdir, 'plot-99th.pdf'), dpi=600)

    p = plot(df, '50th')
    p.savefig(os.path.join(logdir, 'plot-50th.png'), dpi=600)
    p.savefig(os.path.join(logdir, 'plot-50th.pdf'), dpi=600)

if __name__ == '__main__': main()
