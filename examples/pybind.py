import gathering
import numpy as np

gathering.load("cut1_2.obj", 1000, 1200, 1600)
gathering.setSubstepSize(0.03)
gathering.applyForce(10, False, 0.2, 0.2, 0.2)
images = gathering.takeImages(4)
gathering.close()
