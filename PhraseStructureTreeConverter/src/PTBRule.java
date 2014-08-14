import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import edu.stanford.nlp.trees.Tree;

/*
 * The PTB Rule is a string, which basically X -> Y_1 Y_2 ... Y_n
 * This class store this kind of rules and binarize them into CNFs
 * in a way each CNF rule should corresponds to an arc in the dependency
 * parse tree.
 * 
 * Lingpeng Kong, lingpenk@cs.cmu.edu
 * 
 * */

public class PTBRule {
	private List<String> rhs = new ArrayList<String>();
	private int headInd = -1;
	private String lhs = "";

	// This corresponds to the original rule number in the PTB rule set.
	private int ruleInd = 0;

	public PTBRule(Tree tlhs, Tree thead, int ruleInd) {
		lhs = tlhs.nodeString();
		List<Tree> tchildren = tlhs.getChildrenAsList();
		for (Tree t : tchildren) {
			rhs.add(t.nodeString());
		}
		for (int i = 0; i < tchildren.size(); i++) {
			if (tchildren.get(i) == thead) {
				headInd = i;
				break;
			}
		}
		this.ruleInd = ruleInd;
	}

	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + headInd;
		result = prime * result + ((lhs == null) ? 0 : lhs.hashCode());
		result = prime * result + ((rhs == null) ? 0 : rhs.hashCode());
		return result;
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		PTBRule other = (PTBRule) obj;
		if (headInd != other.headInd)
			return false;
		if (lhs == null) {
			if (other.lhs != null)
				return false;
		} else if (!lhs.equals(other.lhs))
			return false;
		if (rhs == null) {
			if (other.rhs != null)
				return false;
		} else if (!rhs.equals(other.rhs))
			return false;
		return true;
	}

	public ArrayList<PTBRule> binRule() {
		if(rhs.size()<=2){
			System.err.println("Binarize a rule with length smaller than or equal to 2!");
			return null;
		}
		ArrayList<PTBRule> brules = new ArrayList<PTBRule>();
		String previousnnt = lhs;
		for(int i = 0; i < headInd-1; i++){
			// leave the left-most one outside
			// others just be one non-terminal
			String nnt = "Z_("+ruleInd + "," + "l" + ","+ i +")";
			ArrayList<String> nrhs = new ArrayList<String>();
			nrhs.add(rhs.get(i));
			nrhs.add(nnt);			
			PTBRule ptbr = new PTBRule(previousnnt, nrhs, 1, ruleInd);
			brules.add(ptbr);
			previousnnt = new String(nnt);
		}
		// the head is the rightmost one
		if (headInd == rhs.size()-1){
			ArrayList<String> nrhs = new ArrayList<String>();
			nrhs.add(rhs.get(headInd-1));
			nrhs.add(rhs.get(headInd));			
			PTBRule ptbr = new PTBRule(previousnnt, nrhs, 1, ruleInd);
			brules.add(ptbr);
			// Everything is done basically
		}else{
			// the head is not the rightmost one
			// left split the last one
			if(headInd >= 1){
				String nnt = "Z_("+ruleInd + "," + "l" + ","+ (headInd-1)  +")";
				ArrayList<String> nrhs = new ArrayList<String>();
				nrhs.add(rhs.get(headInd-1));
				nrhs.add(nnt);			
				PTBRule ptbr = new PTBRule(previousnnt, nrhs, 1, ruleInd);
				brules.add(ptbr);
				previousnnt = new String(nnt);
			}
			for(int i = 0; i < rhs.size() - headInd - 2; i++){
				// leave the right-most one outside
				// others just be one non-terminal
				String nnt = "Z_("+ruleInd + "," + "r" + ","+ i +")";
				ArrayList<String> nrhs = new ArrayList<String>();
				nrhs.add(nnt);
				nrhs.add(rhs.get(rhs.size()-1-i));			
				PTBRule ptbr = new PTBRule(previousnnt, nrhs, 0, ruleInd);
				brules.add(ptbr);
				previousnnt = new String(nnt);
			}
			ArrayList<String> nrhs = new ArrayList<String>();
			nrhs.add(rhs.get(headInd));
			nrhs.add(rhs.get(headInd+1));			
			PTBRule ptbr = new PTBRule(previousnnt, nrhs, 0, ruleInd);
			brules.add(ptbr);
		}
		return brules;
	}
	
	public String toString(){
		String s = "";
		s = s + lhs + "\t-->";
		for(int i = 0; i < rhs.size(); i++){
			if(i == headInd){
				s = s + "\t" + rhs.get(i) + "*";
			}else{
				s = s + "\t" + rhs.get(i);
			}
		}
		s = s + "\t\t" + ruleInd;
		return s;
		
	}

	public PTBRule(String lhs, List<String> list, int headInd, int ruleInd) {
		this.lhs = lhs;
		this.rhs = list;
		this.headInd = headInd;
		this.ruleInd = ruleInd;

	}

	public List<String> getRhs() {
		return rhs;
	}

	public void setRhs(ArrayList<String> rhs) {
		this.rhs = rhs;
	}

	public int getHeadInd() {
		return headInd;
	}

	public void setHeadInd(int headInd) {
		this.headInd = headInd;
	}

	public String getLhs() {
		return lhs;
	}

	public void setLhs(String lhs) {
		this.lhs = lhs;
	}

	public int getRuleInd() {
		return ruleInd;
	}

	public void setRuleInd(int ruleInd) {
		this.ruleInd = ruleInd;
	}

	public static void main(String[] args) {
		PTBRule ptbr = new PTBRule("NP",Arrays.asList(new String[] { "DT", "NNP", "NNP", "CC", "NNP", "NNP" }),3,100);
		ArrayList<PTBRule> set = ptbr.binRule();
		for(PTBRule r : set){
			System.out.println(r.toString());
		}
				
	}

}
