import edu.stanford.nlp.ling.CoreLabel;
import edu.stanford.nlp.parser.lexparser.TreebankLangParserParams;
import edu.stanford.nlp.trees.Tree;
import edu.stanford.nlp.trees.TreeCoreAnnotations;
import edu.stanford.nlp.trees.TreeGraphNode;

//Assume the tree here is a TreeGraphNode, otherwise, this can't be use.

public class TreeHelper {
	public static void BinarizeTree(Tree t, TreebankLangParserParams params){
		if(t.isPreTerminal() || t.isLeaf()){
			// It's done.
			return;
		}else{
			Tree[] originalChildrenList = t.children();
			
			int headPosition = headPosition(t, params);
			TreeGraphNode headNode = (TreeGraphNode)originalChildrenList[headPosition];
			
			// Recursively binarize all the children trees
			for(int i = 0; i < originalChildrenList.length; i++){
				BinarizeTree(originalChildrenList[i], params);
			}
			
			if(originalChildrenList.length <= 2){
				// Then this node do not need to binarize, we are done here.
				return;
			}
			
			// Create a new node
			TreeGraphNode newNode = null;
			
			if(headPosition > 0){
					// At least got one left child
					// Attach the left children one by one
					TreeGraphNode right_child = headNode;
					for(int j = headPosition-1; j >= 0; j--){
						newNode = new TreeGraphNode();
						CoreLabel cl =  new CoreLabel();
						// This is a generated label
						cl.setValue(t.label().value() + "|");
						cl.set(TreeCoreAnnotations.HeadWordAnnotation.class, (headNode.label().get(TreeCoreAnnotations.HeadWordAnnotation.class)));
						newNode.setLabel(cl);
						
						Tree[] childTrees = new Tree[2];
						childTrees[0] = originalChildrenList[j];
						childTrees[1] = right_child;
						newNode.setChildren(childTrees);
						right_child = newNode;
					}
			    }else{
					// No left child
					newNode = headNode;
				}
				
				// The new node should to grab the right children
				TreeGraphNode left_child = newNode;
				for(int j = headPosition+1; j < originalChildrenList.length; j++){
					newNode = new TreeGraphNode();
					CoreLabel cl =  new CoreLabel();
					// This is a generated label
					cl.setValue(t.label().value() + "|");
					cl.set(TreeCoreAnnotations.HeadWordAnnotation.class, (headNode.label().get(TreeCoreAnnotations.HeadWordAnnotation.class)));
					newNode.setLabel(cl);
					Tree[] childTrees = new Tree[2];
					childTrees[0] = left_child;
					childTrees[1] = originalChildrenList[j];
					newNode.setChildren(childTrees);
					left_child = newNode;
				}
				// new node now should have one or children and the children list is the children list of the original node
				t.setChildren(newNode.children());
		}
		return;
		
	}
	
	// First grab right then left, code
	// Create a new node
//				TreeGraphNode newNode = null;
//				
//				if(headPosition < (originalChildrenList.length-1)){
//						// At least got one right child
//						// Attach the right children one by one
//						TreeGraphNode left_child = headNode;
//						for(int j = headPosition+1; j < originalChildrenList.length; j++){
//							newNode = new TreeGraphNode();
//							CoreLabel cl =  new CoreLabel();
//							// This is a generated label
//							cl.setValue(t.label().value() + "|");
//							cl.set(TreeCoreAnnotations.HeadWordAnnotation.class, (headNode.label().get(TreeCoreAnnotations.HeadWordAnnotation.class)));
//							newNode.setLabel(cl);
//							
//							Tree[] childTrees = new Tree[2];
//							childTrees[0] = left_child;
//							childTrees[1] = originalChildrenList[j];
//							newNode.setChildren(childTrees);
//							left_child = newNode;
//						}
//				    }else{
//						// No right child
//						newNode = headNode;
//					}
//					
//					// The new node should to grab the left children
//					TreeGraphNode right_child = newNode;
//					for(int j = headPosition-1; j >= 0; j--){
//						newNode = new TreeGraphNode();
//						CoreLabel cl =  new CoreLabel();
//						// This is a generated label
//						cl.setValue(t.label().value() + "|");
//						cl.set(TreeCoreAnnotations.HeadWordAnnotation.class, (headNode.label().get(TreeCoreAnnotations.HeadWordAnnotation.class)));
//						newNode.setLabel(cl);
//						Tree[] childTrees = new Tree[2];
//						childTrees[0] = originalChildrenList[j];
//						childTrees[1] = right_child;
//						newNode.setChildren(childTrees);
//						right_child = newNode;
//					}
//					// new node now should have one or children and the children list is the children list of the original node
//					t.setChildren(newNode.children());
	
	// Use position when talking about relative positions. Use index only when refer to the index of the words.
	// Return the head word index of this subtree
	
	public static int getHeadNodeInd(Tree t){
		String headWordString = ((TreeGraphNode) (t)).label().get(TreeCoreAnnotations.HeadWordAnnotation.class).toString();
		return Integer.parseInt(headWordString.substring(headWordString.lastIndexOf('-')+1, headWordString.length()));
	}
	
	// Get the head child position in the children list
	public static int headPosition(Tree t, TreebankLangParserParams params){
		Tree head_child = params.typedDependencyHeadFinder().determineHead(t);
		return t.getChildrenAsList().indexOf(head_child);
	}
	
	public static void main(String[] args) {

	}

}
