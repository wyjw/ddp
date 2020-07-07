# pip3 install seaborn==0.9.0

import sys
import os
import os.path
import seaborn as sns
import pandas as pd
import matplotlib.pyplot as plt

def get_data(pth=os.path.join('..', 'results', 'latest', 'output.data')):
    df = pd.read_csv(pth, sep=';')

    # This rounding only adds 1% error on the x-axis and it helps
    # Seaborn realize that nearby points belong to the same cluster
    # even when effective throughput is varying somewhat.
    #df['read_iops']=[int(round(float(v)/1000, 0)*1000) for v in df['read_iops']]
    df.astype({'iops': 'int32'}).dtypes
    df['engine'] = df['engine'].astype('category')
    devs = {'/dev/nvme0n1': 'Intel Optane 900p 280 GB'}
    df['dev'] = [devs[d] for d in df['dev']]
    return df

def plot(df=None, col='99th'):
    if df is None:
        df = get_data()

    sns.set_context('paper')
    sns.set_palette(sns.color_palette('Set1'))

    p = sns.relplot(data=df,
                    x='iops',
                    y=col,
                    hue='engine',
                    kind='line',
                    sort=False,
                    row='dev',
                    ci=None,
                    style='engine',
                    markers=True)
    l = '99th Percentile Latency (us)'
    if col == '50th':
        l = 'Median Latency (us)'
    p = p.set_axis_labels('Throughput (IOPS)', l)
    p = p.set(xlim=(0, 700e3),
              ylim=(0, 100),
              yticks=[i*10 for i in range(0, 11)])
    p.fig.set_size_inches(6, 3.5)

    p._legend.set_title('I/O Engine')
    for t, l in zip(p._legend.texts, ['', 'SPDK', 'AIO']):
        t.set_text(l)

    return p

def plot_iodepth(df=None):
    if df is None:
        df = get_data()

    sns.set_context('paper')
    sns.set_palette(sns.color_palette('Set1'))

    p = sns.relplot(data=df,
                    x='iodepth',
                    y='iops',
                    hue='engine',
                    kind='line',
                    row='dev',
                    ci=None,
                    style='engine',
                    markers=True)
    p = p.set_axis_labels('IO Depth', 'Throughput (IOPS)')
    p = p.set(xlim=(0, 128),
              ylim=(0, 700e3))
    p.fig.set_size_inches(6, 3.5)

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

    p = plot_iodepth(df)
    p.savefig(os.path.join(logdir, 'plot-iodepth.png'))
    p.savefig(os.path.join(logdir, 'plot-iodepth.pdf'))

if __name__ == '__main__': main()
