# -*- coding: utf-8 -*-

from knossos_python_api import *
import NewSkeleton
from NewSkeleton import NewSkeleton as ns

""" This class converts between knossos and the NewSkeleton class """
class Converter:  
    def __init__(self): 
    	# used for temporary saving 		
    	self.comments = list()
    	self.edges = list()

    # at the begin of each conversion the temporary variables are reseted	
    def reset(self):
		self.edges[:] = []
		self.comments[:] = []		

    # this is the method if you want to load a new_skeleton object to knossos	
    def to_knossos(self, new_skeleton):
		self.reset()
		# load trees and nodes
		for annotation in new_skeleton.annotations:
		    	self.load_annotation(annotation)

		# load segments 
		for segment in self.edges:
	    		# 0 = source_id, 1 = target_id	
	        	knossos.add_segment(segment[0], segment[1])	
		
		# load comments
		for comment in self.comments:
	    		# 0 = node_id, 1 = comment
	    		knossos.add_comment(comment[0], comment[1])

		# load branchnode ids
		for branch_node_id in new_skeleton.branchNodes:	   
	    		knossos.set_branch_node(branch_node_id) 

		    
    def load_annotation(self, annotation):	
		knossos.add_tree(annotation.annotation_ID, annotation.comment)	

		# load nodes	
		for node in annotation.nodes:
	    		self.load_skeleton_node(node)

		# temp store the segments
		for source, targets in annotation.edges.iteritems():	
	    		for target in targets:
						self.edges.append((source.ID, target.ID))
   	    		

    def load_skeleton_node(self, skeleton_node):	
		if(not isinstance(skeleton_node, SkeletonNode)):
	    		print ("this node is no instance of SkeletonNode")
	    		return

		if not skeleton_node.data:
	    		print ("data dict does not exist")
	    		return

		node_id = skeleton_node.ID
		tree_id = skeleton_node.annotation.annotation_ID	

		if 'radius' in skeleton_node.data:
	    		radius = skeleton_node.data['radius']
		else:
	    		radius = 1.5
	
		if 'x' in skeleton_node.data:
	    		x = skeleton_node.data['x']
		else:
	    		x = 0
	
		if 'y' in skeleton_node.data:
	    		y = skeleton_node.data['y']	
		else:
	    		y = 0

		if 'z' in skeleton_node.data:
	    		z = skeleton_node.data['z']
		else:
	    		z = 0

		if 'inVp' in skeleton_node.data:
	    		inVp = skeleton_node.data['inVp']
		else:
	    		inVp = 0

		if 'inMag' in skeleton_node.data:
		    	inMag = skeleton_node.data['inMag']
		else:
	    		inMag = 1

		if 'time' in skeleton_node.data:
	    		time = skeleton_node.data['time']
		else:
	    		time = 0

			# print (str(id) + " " + str(radius) + " " + str(x) + " " + str(y) + " " + str(z))	    	
    		knossos.add_node(node_id, x, y, z, tree_id, radius, inVp, inMag, time)
	
		if skeleton_node.pure_comment:	   
	    		self.comments.append((node_id, skeleton_node.pure_comment))	 	    
   
    # this is the method if you want to load the entire knossos skeleton into a new_skeleton object
    def from_knossos(self):
		self.reset()
		skel = ns.NewSkeleton()		

		for tree in knossos.trees():	
	    		annotation = ns.SkeletonAnnotation()
	    		annotation.setComment(tree.comment())

	    		for node in tree.nodes():	
						# as segments are converted after the trees and nodes they will be saved 
						# in a temp storage list 
						for segment in node.segments():
		    					self.edges.append((segment.source().node_id(), segment.target().node_id()))						
			
						skeleton_node = self.convert_knossos_node(node)
			
						if node.is_branch_node():
		    					skel.branchNodes.append(node.node_id())			
						annotation.nodes.add(skeleton_node)
		
																											
	    		# set skeleton annotations
	    		skel.annotations.add(annotation)
	
	
		# set skel activeNode
		skel.activeNode = knossos.active_node()
		return skel

    def convert_knossos_node(self, node):	
		nid = node.node_id() 
		radius = node.radius()
		parent = node.parent_tree()
		if parent:
		    pid = parent.tree_id()	
		pos = node.coordinate()
		x = pos.x()
		y = pos.y()
		z = pos.z()
		vp = node.viewport()
		mag = node.mag()
		time = node.time()

		skeleton_node = ns.SkeletonNode()
		skeleton_node.pure_comment = node.comment()
		skeleton_node.from_scratch(nid, radius, pid, x, y, z, vp, mag, time)	
		return skeleton_node	
			
    def convert_knossos_segment(self, segment):
		source = segment.source()
		target = segment.target()		
	#if source and target:

    def print_segments(self):
		for segment in self.edges:
	    		print str(segment[0]) + " " + str(segment[1])
				
"""

def test_run():
    skel = NewSkeleton()
    skel.fromNml("/home/amos/PyFi/skeleton.nml")

    convert = Converter()
    convert.to_knossos(skel)

    convert.from_knossos()


def main():
    test_run()

if __name__ == "__main__":
    main()

"""
