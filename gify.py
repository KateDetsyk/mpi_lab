import imageio
from os import walk


f = []
folder = "img/"
for (dirpath, dirnames, filenames) in walk(folder):
    f = sorted(filenames, key=lambda x: int(x.split('.')[0]))
    break

with imageio.get_writer('./heat.gif', mode='I') as writer:
    for filename in f:
        image = imageio.imread("{0}{1}".format(folder, filename))
        writer.append_data(image)