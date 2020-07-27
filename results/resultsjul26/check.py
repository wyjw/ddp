import os
import glob
import numpy as np

filenames_un = []
list_of_list_un = []
for filename in glob.glob('*unalt.txt'):
	list_un=[]
	x = 0
	with open(filename, 'r') as f:
		for line in f:
			if 'Total' in line:
				x += int(line.split('Total time: ')[1])
				list_un.append(int(line.split('Total time: ')[1]))
		print(filename + ' ' + str(x/100))
	filenames_un.append(int(filename.split('unalt.txt')[0]))
	list_of_list_un.append(list_un)
	#np.std(np.array(list_un))
    
filenames_alt = []  
list_of_list_alt = []          
for filename in (set(glob.glob('*alt.txt')) - set(glob.glob('*unalt.txt'))):
	list_alt = []
	x = 0
	with open(filename, 'r') as f:
		for line in f:
			if 'Total' in line:
				x += int(line.split('Total time: ')[1])
				list_alt.append(int(line.split('Total time: ')[1]))
		print(filename + ' ' + str(x/100))
	filenames_alt.append(int(filename.split('alt.txt')[0]))
	list_of_list_alt.append(list_alt)
	#np.std(np.array(list_alt))

np_list_un = np.asarray(list_of_list_un)
np_filenames_un = np.asarray(filenames_un)
np_list_alt = np.asarray(list_of_list_alt)
np_filenames_alt = np.asarray(filenames_alt)

np_filenames_a = []
for i in np_filenames_alt:
	np_filenames_a.append([i] * np_list_alt.shape[1])
np_filenames_alt = np_filenames_a

np_filenames_u = []
for i in np_filenames_un:
	np_filenames_u.append([i] * np_list_un.shape[1])
np_filenames_un = np_filenames_u

np_filenames_alt = np.array(np_filenames_alt).flatten()
np_filenames_un = np.array(np_filenames_un).flatten()
np_list_alt = np.array(np_list_alt).flatten()
np_list_un = np.array(np_list_un).flatten()

print(np_filenames_alt)

import pandas as pd

dataset_alt = pd.DataFrame({
	'dep': np_filenames_alt,
	'time': np_list_alt,
	'altered':[True] * len(np_list_alt)
	})

dataset_un = pd.DataFrame({
	'dep': np_filenames_un,
	'time': np_list_un,
	'altered':[False] * len(np_list_un)
})

dataset = pd.concat([dataset_alt, dataset_un])

import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
sns.set(style="whitegrid")

g = sns.catplot(x="dep", y="time", hue="altered", data=dataset,
                height=6, kind="bar", palette="muted")
g.despine(left=True)
plt.show()
'''
dataset_alt.plot.bar(x='dep',y='time', color='blue', ax=ax)
dataset_un.plot.bar(x='dep',y='time', color='red', ax=ax)
'''
#plt.show()