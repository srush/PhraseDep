/*
 * Lingpeng Kong, lingpenk@cs.cmu.edu
 * 
 * */

import java.io.FileReader;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import edu.stanford.nlp.trees.PennTreeReader;
import edu.stanford.nlp.trees.Tree;

public class PhraseStructureTreeConverter {
	public static void ExtractRules(String filePath) throws Exception{
		PennTreeReader ptr = new PennTreeReader(new FileReader(filePath));
		Tree t = ptr.readTree();
		int index = 0;
		HashSet<PTBRule> ruleSet = new HashSet<PTBRule>();
		while (t != null) {
			List<Tree> tlist = t.preOrderNodeList();
			for (Tree st : tlist) {
				if (st.isPhrasal()) {
					List<Tree> childrenList = st.getChildrenAsList();
					System.out.print(st.nodeString() + "\t-->\t");
					Tree head = RuleChecker.findHeader(st);
					PTBRule r = new PTBRule(st, head, index);
					
					if(!ruleSet.contains(r)){
						ruleSet.add(r);
						index++;
					}
					
					for (Tree cst : childrenList) {
						if(cst ==head){
							System.out.print("*");
						}
						System.out.print(cst.nodeString() + "\t");
					}
					System.out.println();
				}
			}
			t = ptr.readTree();
		}
		ptr.close();
		LineWriter lw = new LineWriter("rules");
		for(PTBRule r : ruleSet){
			if(r.getRhs().size() > 2){
				Set<PTBRule> rs = r.binRule();
				for(PTBRule rr : rs){
					lw.writeln(rr.toString());
					System.out.println(rr.toString());
				}
			}else{
				lw.writeln(r.toString());
				System.out.println(r.toString());
			}
		}
		
		lw.closeAll();
		lw = new LineWriter("orules");
		for(PTBRule r : ruleSet){
			lw.writeln(r.toString());
				System.out.println(r.toString());
		}
		lw.closeAll();
	}
	
	public static void ConvertPennTreeToDep(String filePath) throws Exception {
		PennTreeReader ptr = new PennTreeReader(new FileReader(filePath));
		LineWriter lw = new LineWriter("converted");

		// BinaryGrammarExtractor bge = new BinaryGrammarExtractor();
		Tree t = ptr.readTree();
		while (t != null) {

			HashMap<Tree, String> headMap = new HashMap<Tree, String>();

			// Store all the deps as human readable strings
			HashSet<String> depSet = new HashSet<String>();

			// Map the key (child index) to the value (parent index)
			HashMap<Integer, Integer> depMap = new HashMap<Integer, Integer>();

			HashMap<String, String> posMap = new HashMap<String, String>();

			System.out.println(t.toString());

			List<Tree> tlist = t.preOrderNodeList();

			int i = 0;

			// Label the word with index
			List<Tree> leaves = t.getLeaves();
			for (int k = 0; k < leaves.size(); k++) {
				Tree leaf = leaves.get(k);
				leaf.setValue(leaf.nodeString() + "-" + (k + 1));
				System.out.println(leaf);
			}

			// Get the POS for each word
			for (Tree st : tlist) {
				if (st.isPreTerminal()) {
					// System.out.println(st.nodeString() + "\t" +
					// st.firstChild().nodeString());
					posMap.put(st.firstChild().nodeString(), st.nodeString());
				}
			}

			for (Tree st : tlist) {
				labelNodeCollins(st, headMap);
			}

			for (Tree st : tlist) {

				if (st.isPhrasal()) {
					List<Tree> childrenList = st.getChildrenAsList();
					System.out.print(st.nodeString() + "*" + headMap.get(st)
							+ "\t-->\t");
					// for(Tree cst : childrenList){
					// //headMap.put(cst, "gaga" + cst.nodeString());
					// System.out.print(cst.nodeString() +"*"+
					// cst.label().value() + "\t");
					// }
					// System.out.println();
					for (Tree cst : childrenList) {
						System.out.print(cst.nodeString() + "*"
								+ headMap.get(cst) + "\t");
						if (!headMap.get(cst).equals(headMap.get(st))) {
							depSet.add(headMap.get(st) + "\t-->\t"
									+ headMap.get(cst));
							depMap.put(getIndexFromString(headMap.get(cst)),
									getIndexFromString(headMap.get(st)));
						}
					}
					System.out.println("\n" + i + "\t" + st.toString());
				}
				i++;
			}

			for (String ss : depSet) {
				System.out.println(ss);
			}
			for (Tree leaf : leaves) {
				int index = getIndexFromString(leaf.nodeString());
				if (depMap.containsKey(index)) {
					System.out.println(index + "\t<--\t" + depMap.get(index));
				} else {
					// root in this case
					System.out.println(index + "\t<--\t0");
				}
			}

			for (Tree leaf : leaves) {
				int index = getIndexFromString(leaf.nodeString());
				int parent = 0;
				if (depMap.containsKey(index)) {
					parent = depMap.get(index);
				} else {
					// root in this case
					parent = 0;
				}
				String pos = posMap.get(leaf.nodeString());
				String wordForm = getWordFromString(leaf.nodeString());
				lw.writeln(index + "\t" + wordForm + "\t_\t" + pos + "\t" + pos + "\t_\t" + parent + "\t_\t_" );
			}
			lw.writeln();
			t = ptr.readTree();

		}
		lw.closeAll();
		ptr.close();
	}

	// This is a test function that always picks the left-most child as the head
	public static void labelNodeSimple(Tree t, HashMap<Tree, String> headMap) {
		if (t.isLeaf()) {
			headMap.put(t, t.nodeString());
		} else {
			if (!headMap.containsKey(t.getChild(0))) {
				labelNodeSimple(t.getChild(0), headMap);
			}
			headMap.put(t, headMap.get(t.getChild(0)));
		}
	}

	public static void labelNodeCollins(Tree t, HashMap<Tree, String> headMap) {
		if (t.isLeaf()) {
			headMap.put(t, t.nodeString());
		} else {
			Tree head = RuleChecker.findHeader(t);
			if (!headMap.containsKey(head)) {
				labelNodeCollins(head, headMap);
			}
			headMap.put(t, headMap.get(head));
		}
	}

	public static int getIndexFromString(String s) {
		return Integer.parseInt(s.substring(s.lastIndexOf("-") + 1));
	}
	public static String getWordFromString(String s){
		return s.substring(0, s.lastIndexOf("-"));
	}

	public static void main(String[] args) {
		try {
			//ConvertPennTreeToDep("dev.1.notraces");
			ExtractRules("dev.1.notraces");
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

}
