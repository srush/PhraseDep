package old;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import edu.stanford.nlp.trees.Tree;

/*
 * RuleChecker: Check the head rules to match in each case
 * (Now a re-implementation of (Collins, 1999) thesis)
 * 
 * Lingpeng Kong, lingpenk@cs.cmu.edu
 * 
 * */
public class RuleChecker {
	public static final Set<String> PUNCTSET = new HashSet<String>(Arrays.asList(new String[] {
			".", ",", ":", "``", "''" }));
	
	public static Tree findHeader(Tree t) {
		// Leaf, something must go wrong
		if (t.isLeaf()) {
			System.err.println("Finding head for leaf!");
			return null;
		}

		// Unary Rule -> the head must be the only one
		if (t.getChildrenAsList().size() == 1) {
			return t.getChild(0);
		}

		Tree normalHead = findHeaderNormal(t);
		return findHeaderCoord(t, normalHead);

	}

	private static Tree findHeaderNormal(Tree t) {
		// This is an re-implementation of (Collins, 1999) head rules
		// The input is the Tree t X -> Y_1 Y_2 ... Y_n (as in the first level,
		// the tree can be deeper, but we only care about the first level in
		// this function)

		// Rules for NPs
		if (t.nodeString().equals("NP")) {
			// If the last word is tagged POS return (last word)
			if (lastNoPunctChild(t).nodeString().equals("POS")) {
				return lastNoPunctChild(t);
			}
			// Else search from right to left for the first child which is an
			// NN, NNP, NNPS, NNS, NX, POS or JJR
			List<Tree> tchildren = t.getChildrenAsList();
			Set<String> s = new HashSet<String>(Arrays.asList(new String[] {
					"NN", "NNP", "NNPS", "NNS", "NX", "POS", "JJR" }));
			for (int i = tchildren.size() - 1; i >= 0; i--) {
				if (s.contains(t.getChild(i).nodeString())) {
					return t.getChild(i);
				}
			}
			// Else search from left to right for the first child which is an NP
			for (int i = 0; i < tchildren.size(); i++) {
				if (t.getChild(i).nodeString().equals("NP")) {
					return t.getChild(i);
				}
			}
			// Else search from right to left for the first child which is a $,
			// ADJP or PRN.
			s = new HashSet<String>(Arrays.asList(new String[] { "$", "ADJP",
					"PRN" }));
			for (int i = tchildren.size() - 1; i >= 0; i--) {
				if (s.contains(t.getChild(i).nodeString())) {
					return t.getChild(i);
				}
			}
			// Else search from right to left for the first child which is a CD.
			for (int i = tchildren.size() - 1; i >= 0; i--) {
				if (t.getChild(i).nodeString().equals("CD")) {
					return t.getChild(i);
				}
			}
			// Else search from right to left for the first child which is a JJ,
			// JJS, RB or QP.
			s = new HashSet<String>(Arrays.asList(new String[] { "JJ", "JJS",
					"RB", "QP" }));
			for (int i = tchildren.size() - 1; i >= 0; i--) {
				if (s.contains(t.getChild(i).nodeString())) {
					return t.getChild(i);
				}
			}
			// Else return the last word.
			return lastNoPunctChild(t);
		}
		// ADJP -- Left -- NNS QP NN $ ADVP JJ VBN VBG ADJP JJR NP JJS DT FW RBR
		// RBS SBAR RB
		List<Tree> tchildren = t.getChildrenAsList();
		if (t.nodeString().equals("ADJP")) {
			List<String> plist = Arrays.asList(new String[] { "NNS", "QP",
					"NN", "$", "ADVP", "JJ", "VBN", "VBG", "ADJP", "JJR", "NP",
					"JJS", "DT", "FW", "RBR", "RBS", "SBAR", "RB" });
			for (String pe : plist) {
				for (int i = 0; i < tchildren.size(); i++) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return leftmost
			return firstNoPunctChild(t);
		}

		// ADVP -- Right -- RB RBR RBS FW ADVP TO CD JJR JJ IN NP JJS NN
		if (t.nodeString().equals("ADVP")) {
			List<String> plist = Arrays.asList(new String[] { "RB", "RBR",
					"RBS", "FW", "ADVP", "TO", "CD", "JJR", "JJ", "IN", "NP",
					"JJS", "NN" });
			for (String pe : plist) {
				for (int i = tchildren.size() - 1; i >= 0; i--) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return rightmost
			return lastNoPunctChild(t);
		}

		// CONJP -- Right -- CC RB IN
		if (t.nodeString().equals("CONJP")) {
			List<String> plist = Arrays
					.asList(new String[] { "CC", "RB", "IN" });
			for (String pe : plist) {
				for (int i = tchildren.size() - 1; i >= 0; i--) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return rightmost
			return lastNoPunctChild(t);
		}

		// FRAG -- Right
		if (t.nodeString().equals("FRAG")) {
			return lastNoPunctChild(t);
		}

		// INTJ -- Left
		if (t.nodeString().equals("INTJ")) {
			return firstNoPunctChild(t);
		}

		// LST -- Right -- LS :
		if (t.nodeString().equals("LST")) {
			List<String> plist = Arrays.asList(new String[] { "LS", ":" });
			for (String pe : plist) {
				for (int i = tchildren.size() - 1; i >= 0; i--) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return rightmost
			return lastNoPunctChild(t);
		}
		// NAC -- Left -- NN NNS NNP NNPS NP NAC EX $ CD QP PRP VBG JJ JJS JJR
		// ADJP FW
		if (t.nodeString().equals("NAC")) {
			List<String> plist = Arrays.asList(new String[] { "NN", "NNS",
					"NNP", "NNPS", "NP", "NAC", "EX", "$", "CD", "QP", "PRP",
					"VBG", "JJ", "JJS", "JJR", "ADJP", "FW" });
			for (String pe : plist) {
				for (int i = 0; i < tchildren.size(); i++) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return leftmost
			return firstNoPunctChild(t);
		}
		// PP -- Right -- IN TO VBG VBN RP FW
		if (t.nodeString().equals("PP")) {
			List<String> plist = Arrays.asList(new String[] { "IN", "TO",
					"VBG", "VBN", "RP", "FW" });
			for (String pe : plist) {
				for (int i = tchildren.size() - 1; i >= 0; i--) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return rightmost
			return lastNoPunctChild(t);
		}
		// PRN -- Left
		if (t.nodeString().equals("PRN")) {
			return firstNoPunctChild(t);
		}
		// PRT -- Right -- RP
		if (t.nodeString().equals("PRT")) {
			List<String> plist = Arrays.asList(new String[] { "RP" });
			for (String pe : plist) {
				for (int i = tchildren.size() - 1; i >= 0; i--) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return rightmost
			return lastNoPunctChild(t);
		}
		// QP -- Left -- $ IN NNS NN JJ RB DT CD NCD QP JJR JJS
		if (t.nodeString().equals("QP")) {
			List<String> plist = Arrays.asList(new String[] { "$", "IN", "NNS",
					"NN", "JJ", "RB", "DT", "CD", "NCD", "QP", "JJR", "JJS" });
			for (String pe : plist) {
				for (int i = 0; i < tchildren.size(); i++) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return leftmost
			return firstNoPunctChild(t);
		}
		// RRC -- Right -- VP NP ADVP ADJP PP
		if (t.nodeString().equals("RRC")) {
			List<String> plist = Arrays.asList(new String[] { "VP","NP","ADVP","ADJP","PP" });
			for (String pe : plist) {
				for (int i = tchildren.size() - 1; i >= 0; i--) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return rightmost
			return lastNoPunctChild(t);
		}
		// S -- Left -- TO IN VP S SBAR ADJP UCP NP
		if (t.nodeString().equals("S")) {
			List<String> plist = Arrays.asList(new String[] { "TO", "IN", "VP",
					"S", "SBAR", "ADJP", "UCP", "NP" });
			for (String pe : plist) {
				for (int i = 0; i < tchildren.size(); i++) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return leftmost
			return firstNoPunctChild(t);
		}
		// SBAR -- Left -- WHNP WHPP WHADVP WHADJP IN DT S SQ SINV SBAR FRAG
		if (t.nodeString().equals("SBAR")) {
			List<String> plist = Arrays.asList(new String[] { "WHNP", "WHPP",
					"WHADVP", "WHADJP", "IN", "DT", "S", "SQ", "SINV", "SBAR",
					"FRAG" });
			for (String pe : plist) {
				for (int i = 0; i < tchildren.size(); i++) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return leftmost
			return firstNoPunctChild(t);
		}
		// SBARQ -- Left -- SQ S SINV SBARQ FRAG
		if (t.nodeString().equals("SBARQ")) {
			List<String> plist = Arrays.asList(new String[] { "SQ", "S", "SINV", "SBARQ", "FRAG" });
			for (String pe : plist) {
				for (int i = 0; i < tchildren.size(); i++) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return leftmost
			return firstNoPunctChild(t);
		}
		// SINV -- Left -- VBZ VBD VBP VB MD VP S SINV ADJP NP
		if (t.nodeString().equals("SINV")) {
			List<String> plist = Arrays.asList(new String[] { "VBZ", "VBD",
					"VBP", "VB", "MD", "VP", "S", "SINV", "ADJP", "NP" });
			for (String pe : plist) {
				for (int i = 0; i < tchildren.size(); i++) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return leftmost
			return firstNoPunctChild(t);
		}
		// SQ -- Left -- VBZ VBD VBP VB MD VP SQ
		if (t.nodeString().equals("SQ")) {
			List<String> plist = Arrays.asList(new String[] { "VBZ", "VBD",
					"VBP", "VB", "MD", "VP", "SQ" });
			for (String pe : plist) {
				for (int i = 0; i < tchildren.size(); i++) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return leftmost
			return firstNoPunctChild(t);
		}
		// UCP -- Right
		if (t.nodeString().equals("UCP")) {
			return lastNoPunctChild(t);
		}
		// VP -- Left -- TO VBD VBN MD VBZ VB VBG VBP VP ADJP NN NNS NP
		if (t.nodeString().equals("VP")) {
			List<String> plist = Arrays.asList(new String[] { "TO", "VBD",
					"VBN", "MD", "VBZ", "VB", "VBG", "VBP", "VP", "ADJP", "NN",
					"NNS", "NP" });
			for (String pe : plist) {
				for (int i = 0; i < tchildren.size(); i++) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return leftmost
			return firstNoPunctChild(t);
		}
		// WHADJP -- Left -- CC WRB JJ ADJP
		if (t.nodeString().equals("WHADJP")) {
			List<String> plist = Arrays.asList(new String[] { "CC", "WRB", "JJ", "ADJP" });
			for (String pe : plist) {
				for (int i = 0; i < tchildren.size(); i++) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return leftmost
			return firstNoPunctChild(t);
		}
		// WHADVP -- Right -- CC WRB
		if (t.nodeString().equals("WHADVP")) {
			List<String> plist = Arrays.asList(new String[] { "CC", "WRB" });
			for (String pe : plist) {
				for (int i = tchildren.size() - 1; i >= 0; i--) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return rightmost
			return lastNoPunctChild(t);
		}
		// WHNP -- Left -- WDT WP WP$ WHADJP WHPP WHNP
		if (t.nodeString().equals("WHNP")) {
			List<String> plist = Arrays.asList(new String[] { "WDT", "WP",
					"WP$", "WHADJP", "WHPP", "WHNP" });
			for (String pe : plist) {
				for (int i = 0; i < tchildren.size(); i++) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return leftmost
			return firstNoPunctChild(t);
		}
		// WHPP -- Right -- IN TO FW
		if (t.nodeString().equals("WHPP")) {
			List<String> plist = Arrays.asList(new String[] { "IN", "TO", "FW" });
			for (String pe : plist) {
				for (int i = tchildren.size() - 1; i >= 0; i--) {
					if (t.getChild(i).nodeString().equals(pe)) {
						return t.getChild(i);
					}
				}
			}
			// Nothing found, return rightmost
			return lastNoPunctChild(t);
		}
		
		// No rule found, return leftmost
		return firstNoPunctChild(t);
	}

	private static Tree findHeaderCoord(Tree t, Tree normalHead) {
		// Rules for Coordinated Phrases
		List<Tree> tchildren = t.getChildrenAsList();
		int h = tchildren.indexOf(normalHead);
		if( h < tchildren.size() - 2){
			if(tchildren.get(h+1).nodeString().equals("CC")){
				// Y_h Y_h+1 Y_h+2 forms a triple of non-terminals in a coordinating relationship,
				// But since Y_h already the head, nothing happened in unlabeled parsing.
				return normalHead;
			}
		}
		if( h > 1){
			if(tchildren.get(h-1).nodeString().equals("CC")){
				// Y_h-2 Y_h-1 Y_h forms a triple of non-terminals in a coordinating relationship,
				// the head is modified to be Y_h-2 in this case

				// Make sure punctuation not selected as the head
				if(!PUNCTSET.contains(tchildren.get(h-2).nodeString())){
					return tchildren.get(h-2);
				}
			}
		}
		return normalHead;
	}
	
	private static Tree firstNoPunctChild(Tree t){
		for(int i = 0; i < t.getChildrenAsList().size(); i++){
			if(!PUNCTSET.contains(t.getChild(i).nodeString())){
				return t.getChild(i);
			}
		}
		return t.firstChild();
	}
	
	private static Tree lastNoPunctChild(Tree t){
		for(int i = t.getChildrenAsList().size()-1; i >= 0; i--){
			if(!PUNCTSET.contains(t.getChild(i).nodeString())){
				return t.getChild(i);
			}
		} 
		return t.lastChild();
	}

	public static void main(String[] args) {

	}

}
