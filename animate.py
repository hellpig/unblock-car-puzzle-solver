#!/usr/bin/env python3.8
#
# Animates the output.txt that solve.cpp produced
#
# Close the animation figure to enter interactive mode!


import matplotlib.pyplot as plt
from matplotlib import animation
from matplotlib.widgets import Button


filename = 'output.txt'
figScale = 0.7
delay = 250    # milliseconds for each frame of animation


# read in the text file
f = open(filename, 'rt')
siz = eval(f.readline())
numCars = eval(f.readline())
carDataInitial = []
for i in range(numCars):
  carDataInitial.append(eval(f.readline()))
solution = eval(f.readline())
wallData = eval(f.readline())
f.close()

# calculate a couple things
winCar = solution[-3]
moves = len(solution)//3



def setupFigure():

    fig, ax = plt.subplots()
    ax.axis('off')
    fig.set_size_inches( figScale*siz[1], figScale*(siz[0]+2) )
    ann = ax.annotate('move 0', (0.1, 1 - 0.8/(siz[0]+2)), xycoords = 'figure fraction')

    for i in wallData:
        row = i // siz[1]
        col = i % siz[1]
        rect = plt.Rectangle((col/siz[1], 1 - (row+2)/(siz[0]+2)), 1/siz[1], 1/(siz[0]+2),
           color = (0.5, 0.5, 0.5))
        fig.add_artist(rect)

    rects = []
    for i in range(numCars):
        if carDataInitial[i][3]:   # vertical
            row = carDataInitial[i][1] + carDataInitial[i][4] + s[i] + 1
            col = carDataInitial[i][2]
            width = 1
            height = carDataInitial[i][4]
        else:
            row = carDataInitial[i][1] + 2
            col = carDataInitial[i][2] + s[i]
            width = carDataInitial[i][4]
            height = 1
        if i==winCar:
            colorRect = (1, 0, 0)
        else:
            colorRect = (0, 0, 0)
        rects.append( plt.Rectangle(( (col+0.1)/siz[1], 1-(row-0.1)/(siz[0]+2) ),
          (width-0.2)/siz[1], (height-0.2)/(siz[0]+2), color = colorRect, fill = False) )
        fig.add_artist(rects[-1])

    return [fig, ax, ann, rects]



def update(car):

    ann.set_text('move ' + str(j))

    if carDataInitial[car][3]:   # vertical
        row = carDataInitial[car][1] + carDataInitial[car][4] + s[car] + 1
        col = carDataInitial[car][2]
    else:
        row = carDataInitial[car][1] + 2
        col = carDataInitial[car][2] + s[car]
    rects[car].set_xy( ( (col+0.1)/siz[1], 1-(row-0.1)/(siz[0]+2) ) )

    plt.draw()





## do a quick animation to start things off

j = 0
s = [0]*numCars     # this is the same as wrapped.s[] in solve.cpp except *down* and right are positive
[fig, ax, ann, rects] = setupFigure()

ax.annotate('Solved!', (0.2, 0.2/(siz[0]+2)), xycoords = 'figure fraction')

def animateFunc(frame):
    global j
    i = frame - 1   # start with a brief pause to display the original puzzle
    if i < 0:
      return
    j += 1
    step = [solution[3*i], solution[3*i + 1], solution[3*i + 2]]   # [car, direction, count]
    s[step[0]] += ( ((step[1]-1) % 2)*2 - 1)*step[2]
    update(step[0])
    return

_ = animation.FuncAnimation(fig, animateFunc, frames = moves+1, interval=delay, repeat=False)
plt.show()




##  give buttons to make figure interactive

j = 0
s = [0]*numCars
[fig, ax, ann, rects] = setupFigure()

def buttonFuncLeft(var):
    global j
    if j>0:
        j -= 1
        step = [solution[3*j], solution[3*j + 1], solution[3*j + 2]]
        s[step[0]] -= ( ((step[1]-1) % 2)*2 - 1)*step[2]
        update(step[0])

def buttonFuncRight(var):
    global j
    if j<moves:
        step = [solution[3*j], solution[3*j + 1], solution[3*j + 2]]
        j += 1
        s[step[0]] += ( ((step[1]-1) % 2)*2 - 1)*step[2]
        update(step[0])

# make the buttons
xStart = 0.1
width = 1.0
yPadding = 0.3
dim = [xStart, yPadding/(siz[0]+2), width/siz[1], (1-2*yPadding)/(siz[0]+2)]
buttonLeft = Button( plt.axes(dim), '<--')
dim = [1 - xStart - width/siz[1], yPadding/(siz[0]+2), width/siz[1], (1-2*yPadding)/(siz[0]+2)]
buttonRight = Button( plt.axes(dim), '-->')
buttonLeft.on_clicked(buttonFuncLeft)
buttonRight.on_clicked(buttonFuncRight)

plt.show()
