/*
 * Lingpeng Kong, lingpenk@cs.cmu.edu
 * 
 * */

import java.io.FileReader;
import java.util.HashMap;
import java.util.List;

import edu.stanford.nlp.trees.PennTreeReader;
import edu.stanford.nlp.trees.Tree;



public class PhraseStructureTreeConverter {
	public static void readPennTree(String filePath) throws Exception {
		PennTreeReader ptr = new PennTreeReader(new FileReader(filePath));
		
		// BinaryGrammarExtractor bge = new BinaryGrammarExtractor();
		Tree t = ptr.readTree();
//		while(t!=null){
			
			HashMap<Tree, String> headMap = new HashMap<Tree, String>();
			
			System.out.println(t.toString());
			
			List<Tree> tlist = t.preOrderNodeList();
			for(Tree st : tlist){
				labelNodeCollins(st, headMap);
			}
			int i = 0;
			
			for(Tree st : tlist){
				
				if (st.isPhrasal()){
					List<Tree> childrenList = st.getChildrenAsList();
					System.out.print(st.nodeString() +"*"+ headMap.get(st)  + "\t-->\t");
//					for(Tree cst : childrenList){
//						//headMap.put(cst, "gaga" + cst.nodeString());
//						System.out.print(cst.nodeString() +"*"+ cst.label().value() + "\t");
//					}
					//System.out.println();
					for(Tree cst : childrenList){
						System.out.print(cst.nodeString() +"*"+ headMap.get(cst) + "\t");
					}
					System.out.println("\n" + i + "\t" + st.toString());
				}
				i++;
			}
			
			
			t = ptr.readTree();
			
//		}
		
		ptr.close();
	}
	
	// This is a test function that always picks the left-most child as the head
	public static void labelNodeSimple(Tree t, HashMap<Tree, String> headMap) {
		if (t.isLeaf()) {
			headMap.put(t, t.nodeString());
		} else {
			if(!headMap.containsKey(t.getChild(0))){
				labelNodeSimple(t.getChild(0), headMap);
			}
			headMap.put(t, headMap.get(t.getChild(0)));
		}
	}
	
	public static void labelNodeCollins(Tree t, HashMap<Tree, String> headMap){
		if (t.isLeaf()) {
			headMap.put(t, t.nodeString());
		} else {
			Tree head = RuleChecker.findHeader(t);
			if(!headMap.containsKey(head)){
				labelNodeCollins(head, headMap);
			}
			headMap.put(t, headMap.get(head));
		}
	}
	
	public static void main(String[] args) {
		try {
			readPennTree("dev.1.notraces");
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

}
