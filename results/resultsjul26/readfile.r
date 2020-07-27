temp_unalt = list.files(pattern="*unalt.txt")
myfiles_unalt = lapply(temp, read.delim)

temp_alt = list.files(pattern="*alt.txt")
myfiles_alt = lapply(temp, read.delim)

