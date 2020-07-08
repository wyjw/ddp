library(ggplot2)

loadDataFile <- function (device, procs, polling='FALSE') {
  d <- read.table(paste0(device, '-', procs, '-', polling, '.data'), header=F)
  
  model = 'Unknown'
  if (device == 'SSDPED1D280GA') {
    model = 'Intel 900p 280 GB Optane'
  } else if (device == 'SSDPEKNW512G8') {
    model = 'Intel 660p 512 GB SLC+QLC NAND Flash'
  }
  
  d <- cbind(d, rep(device, nrow(d)))
  d <- cbind(d, rep(model, nrow(d)))
  d <- cbind(d, rep(procs, nrow(d)))
  d <- cbind(d, rep(F, nrow(d)))
  
  colnames(d) <- c('ns', 'device', 'model', 'procs', 'polling')
  
  d
}

loadAllData <- function () {
  df <- NULL
  for (polling in c('FALSE')) {
    for (d in c('SSDPED1D280GA', 'SSDPEKNW512G8')) {
     for (p in c(1, 2, 4, 6, 8, 10)) {
        df <- rbind(df, loadDataFile(d, p, polling))
      }
    }
  }
  df
}

# d1 <- read.table('1-proc.data', header=F)
# d1 <- cbind(d1, rep(1, nrow(d1)))
# d1 <- cbind(d1, rep(F, nrow(d1)))
# colnames(d1) <- c('ns', 'nprocs', 'polling')
# 
# d3 <- read.table('3-proc.data', header=F)
# d3 <- cbind(d3, rep(3, nrow(d3)))
# d3 <- cbind(d3, rep(F, nrow(d3)))
# colnames(d3) <- c('ns', 'nprocs', 'polling')
# 
# d6 <- read.table('6-proc.data', header=F)
# d6 <- cbind(d6, rep(6, nrow(d6)))
# d6 <- cbind(d6, rep(F, nrow(d6)))
# colnames(d6) <- c('ns', 'nprocs', 'polling')
# 
# d12 <- read.table('12-proc.data', header=F)
# d12 <- cbind(d12, rep(12, nrow(d12)))
# d12 <- cbind(d12, rep(F, nrow(d12)))
# colnames(d12) <- c('ns', 'nprocs', 'polling')
# 
# d1p <- read.table('1-proc-poll.data', header=F)
# d1p <- cbind(d1p, rep(1, nrow(d1p)))
# d1p <- cbind(d1p, rep(T, nrow(d1p)))
# colnames(d1p) <- c('ns', 'nprocs', 'polling')
# 
# d4p <- read.table('4-proc-poll.data', header=F)
# d4p <- cbind(d4p, rep(4, nrow(d4p)))
# d4p <- cbind(d4p, rep(T, nrow(d4p)))
# colnames(d4p) <- c('ns', 'nprocs', 'polling')
# 
# d1h <- read.table('1-proc-r100000.data', header=F)
# d1h <- cbind(d1h, rep(99, nrow(d1h)))
# d1h <- cbind(d1h, rep(F, nrow(d1h)))
# colnames(d1h) <- c('ns', 'nprocs', 'polling')


#d <- rbind(d1, d3, d6, d12, d1p, d4p, d1h)

d <- loadAllData()
d$us <- d$ns / 1000

d$procs <- factor(d$procs)

p <- ggplot(d, aes(x=us, color=procs, linetype=polling)) +
  facet_grid(.~model) +
  stat_ecdf(size=0.8) +
  theme_bw() +
  scale_y_continuous(name='Cumulative Fraction',
                     breaks=(0:10)/10.0) +
  scale_x_log10(name='I/O Response Time (us)') +
  annotation_logticks(sides='b') +
  scale_color_brewer(name='Processes',
                     palette = 'Set1') +
  coord_cartesian(xlim=c(8, 1000),
                  ylim=c(0, 1)) +
  theme(panel.grid.minor.x = element_blank())