import gathering

gathering.load("cut1_2.obj", 200, 600, 800)
gathering.setSubstepSize(0.04)
gathering.applyForce(50, False, 0.2,0.2,0.2)
gathering.applyForce(50, False, -0.2,-0.2,-0.2)
