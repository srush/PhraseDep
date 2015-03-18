import java.io.FileReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Properties;
import java.util.Stack;

import edu.stanford.nlp.ling.Label;
import edu.stanford.nlp.parser.lexparser.TreebankLangParserParams;
import edu.stanford.nlp.trees.GrammaticalStructure;
import edu.stanford.nlp.trees.PennTreeReader;
import edu.stanford.nlp.trees.Tree;
import edu.stanford.nlp.trees.TreeGraphNode;
import edu.stanford.nlp.trees.TypedDependency;
import edu.stanford.nlp.util.Filters;
import edu.stanford.nlp.util.IntPair;
import edu.stanford.nlp.util.ReflectionLoading;
import edu.stanford.nlp.util.StringUtils;


public class StanfordHeadRuleFinder {
	public static ArrayList<Tree> readTrees(String filepath) {
		ArrayList<Tree> trees = new ArrayList<Tree>();
		try {
			PennTreeReader ptr = new PennTreeReader(new FileReader(filepath));
			Tree t = ptr.readTree();
			while (t != null) {
				trees.add(t);
				t = ptr.readTree();
			}
			ptr.close();
		} catch (Exception e) {
			e.printStackTrace();
		}
		return trees;
	}
	
	public static HashMap<String, Integer> generateRules(ArrayList<Tree> trees, TreebankLangParserParams params, boolean use_back_off_rule, LineWriter lw){
		HashMap<String, Integer> ruleMap = new HashMap<String, Integer>();
		// Basically, a rule is a string of "rule_id is_unary NT_left NT_right_1 (NT_right_2) (head_side)"
		// Where (NT_right_2) and (head_side) only occur if it is not a unary rule
		HashSet<String> full_rule_set = new HashSet<String>();
		HashSet<String> nt_set = new HashSet<String>();
		HashSet<String> pos_set = new HashSet<String>();
		
		for(Tree t : trees){
			GrammaticalStructure gs = params.getGrammaticalStructure(t, Filters.acceptFilter(), params.typedDependencyHeadFinder());
			TreeGraphNode root = gs.root();
			TreeHelper.BinarizeTree(root, params);
			
			Stack<Tree> stack = new Stack<Tree>();
			stack.push(root);
			while(!stack.isEmpty()){
				TreeGraphNode node = (TreeGraphNode)stack.pop();
				String rule = getRuleAt(node);
				full_rule_set.add(rule);
				for(Tree child : node.children()){
					if(child.isPreTerminal()){
						if(child.isLeaf()) continue;
						pos_set.add(child.label().value());
					}else{
						stack.push(child);
					}
				}
			}
		}
	    // Generate the back-off rules
	    HashSet<String> backoff_rule_set = new HashSet<String>();
	    for (String r : full_rule_set){
	        String[] args = r.split(" ");
	        nt_set.add(args[1]);
	        for(int i = 1; i < args.length -1; i++){
	            if (pos_set.contains(args[i])){
	            	// TODO: CHECK IF IT IS A DEEP COPY HERE
	                String[] args_copy = Arrays.copyOf(args,args.length);
	                args_copy[i] = "BPOS|";
	                backoff_rule_set.add(String.join(" ", args_copy));
	            }
	        }
	    }


	    int ind = 0;
	    for (String r : full_rule_set){
	    	ruleMap.put(r, ind);
	        lw.writeln(ind + " " + r);
	        ind += 1;
	    }
	    
	    if(use_back_off_rule){
	        for(String r : backoff_rule_set){
	        	//ruleMap.put(r, ind);
	        	lw.writeln(ind + " " + r);
	            ind += 1;
	        }
	        for (String pos : pos_set){
	        	//ruleMap.put("1 BPOS| " + pos, ind);
	        	lw.writeln(ind + " 1 BPOS| " + pos);
	            ind += 1;
	        }
	    }
	    return ruleMap;
	}
	
	public static void generateParts(ArrayList<Tree> trees, TreebankLangParserParams params, HashMap<String, Integer> ruleMap, LineWriter lw){
		for(Tree t : trees){
			GrammaticalStructure gs = params.getGrammaticalStructure(t, Filters.acceptFilter(), params.typedDependencyHeadFinder());
			TreeGraphNode root = gs.root();
			
			ArrayList<String> tokens = new ArrayList<String>();
			ArrayList<String> poss = new ArrayList<String>();
			
			// Get the tokens and the POS
			for (Tree leaf: root.getLeaves()){
				tokens.add(leaf.label().value());
			}
			for (Label pos : root.preTerminalYield()){
				poss.add(pos.value());
			}
			int sentence_length = poss.size();
			
			int[] headList = new int[sentence_length];
			
			ArrayList<String> relnList = new ArrayList<String>();
			// Get the dependencies and their corresponding labels
			for(TypedDependency td : gs.typedDependencies(false)){
				// We use root = -1, first word index is 0, so -1 here
				int head = td.gov().index() - 1;
				int child = td.dep().index() - 1;
				headList[child] = head;	
				relnList.add(td.reln().toString());
			}
			ArrayList<String> headListString = new ArrayList<String>();
			for(int h : headList){
				headListString.add(""+h);
			}
			
			// Now finally, we start to generate the parts.
			TreeHelper.BinarizeTree(root, params);

			ArrayList<String> parts = new ArrayList<String>();
			
			// Go through every non-terminal node in the tree.
			Stack<Tree> stack = new Stack<Tree>();
			stack.push(root);
			root.setSpans();
			while(!stack.isEmpty()){
				TreeGraphNode node = (TreeGraphNode)stack.pop();
				String rule = getRuleAt(node);
				
				// This produce i and k
				IntPair span = node.getSpan();
				int i = span.get(0);
				int k = span.get(1);
				
				//This produce j
				int j = node.firstChild().getSpan().get(1);
				
				int head_position = TreeHelper.getHeadNodeInd(node) - 1;
				int left_head = TreeHelper.getHeadNodeInd(node.firstChild()) - 1;
				int right_head = TreeHelper.getHeadNodeInd(node.lastChild()) - 1;				
				
				int h = head_position;
				int m = head_position == left_head ? right_head : left_head;
				
				int rule_num = ruleMap.get(rule);
				
				for(Tree child : node.children()){
					if(child.isPreTerminal() || child.isLeaf()){
						continue;
					}else{
						stack.push(child);
					}
				}

				String part = "" + i + " " + j + " " + k + " " + h + " " + m + " " + rule_num;
				parts.add(part);
			}
			int parts_num = parts.size();
			lw.writeln(sentence_length + " " + parts_num);
			lw.writeln(String.join(" ", tokens));
			lw.writeln(String.join(" ", poss));
			lw.writeln(String.join(" ", headListString));
			lw.writeln(String.join(" ", relnList));
			for(String part : parts){
				lw.writeln(part);
			}
		}
	}
	
	private static String getRuleAt(Tree node){
		String nt = node.label().value();
		boolean is_unary = (node.children().length == 1);
		String rule = null;
		if(is_unary){
			String nt1 = node.children()[0].label().value();
			rule = "1 " + nt + " " + nt1;	
		}else{
			String nt1 = node.children()[0].label().value();
			String nt2 = node.children()[1].label().value();
			int head_side = (TreeHelper.getHeadNodeInd(node) == TreeHelper.getHeadNodeInd(node.children()[0])) ? 0 : 1;
			rule = "0 " + nt + " " + nt1 + " " + nt2 + " " + head_side;
		}
		return rule;
	}
	
	public static void generateConll(ArrayList<Tree> trees, TreebankLangParserParams params){
		for(Tree t : trees){
			GrammaticalStructure gs = params.getGrammaticalStructure(t, Filters.acceptFilter(), params.typedDependencyHeadFinder());
			GrammaticalStructure.printDependencies(gs, gs.typedDependencies(false), t, true, false);
		}
	}
	
	public static void main(String[] args){
		Properties props = StringUtils.argsToProperties(args);
		String treeFileName = props.getProperty("treeFile");
		ArrayList<Tree> trees = readTrees(treeFileName);
		TreebankLangParserParams params = ReflectionLoading.loadByReflection("edu.stanford.nlp.parser.lexparser.EnglishTreebankParserParams");
	    
		boolean extractDependencies = props.getProperty("extractDep") != null;
		if(extractDependencies){
			generateConll(trees,params);
		}else{
			String outputDir = props.getProperty("outputDir");    
			LineWriter lw = new LineWriter(outputDir + "/rules");
		    HashMap<String, Integer> ruleMap = generateRules(trees, params, true, lw);
		    lw.closeAll();
		    
		    lw = new LineWriter(outputDir + "/parts");
		    generateParts(trees, params, ruleMap, lw);
		    lw.closeAll();
		}
	}
}

