import java.io.FileReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Properties;
import java.util.Stack;

import edu.stanford.nlp.parser.lexparser.TreebankLangParserParams;
import edu.stanford.nlp.trees.GrammaticalStructure;
import edu.stanford.nlp.trees.PennTreeReader;
import edu.stanford.nlp.trees.Tree;
import edu.stanford.nlp.trees.TreeCoreAnnotations;
import edu.stanford.nlp.trees.TreeGraphNode;
import edu.stanford.nlp.util.Filters;
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
				String nt = node.label().value();
				boolean is_unary = (node.children().length == 1);
				if(is_unary){
					String nt1 = node.children()[0].label().value();
					String rule = "1 " + nt + " " + nt1;
					full_rule_set.add(rule);
				}else{
					String nt1 = node.children()[0].label().value();
					String nt2 = node.children()[1].label().value();
					int head_side = (TreeHelper.getHeadNodeInd(node) == TreeHelper.getHeadNodeInd(node.children()[0])) ? 0 : 1;
					String rule = "0 " + nt + " " + nt1 + " " + nt2 + " " + head_side;
					full_rule_set.add(rule);
				}
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
			TreeHelper.BinarizeTree(root, params);
			
			Stack<Tree> stack = new Stack<Tree>();
			stack.push(root);
			while(!stack.isEmpty()){
				TreeGraphNode node = (TreeGraphNode)stack.pop();
				String nt = node.label().value();
				boolean is_unary = (node.children().length == 1);
				if(is_unary){
					String nt1 = node.children()[0].label().value();
					String rule = "1 " + nt + " " + nt1;
					
				}else{
					String nt1 = node.children()[0].label().value();
					String nt2 = node.children()[1].label().value();
					int head_side = (TreeHelper.getHeadNodeInd(node) == TreeHelper.getHeadNodeInd(node.children()[0])) ? 0 : 1;
					String rule = "0 " + nt + " " + nt1 + " " + nt2 + " " + head_side;
					
				}
				//TODO
			}
		}
	}
	
	public static void generateConll(ArrayList<Tree> trees, TreebankLangParserParams params){
		for(Tree t : trees){
			GrammaticalStructure gs = params.getGrammaticalStructure(t, Filters.acceptFilter(), params.typedDependencyHeadFinder());
			//System.out.println(gs.toString());
			GrammaticalStructure.printDependencies(gs, gs.typedDependencies(false), t, true, false);
//			break;
		}
	}
	
	public static void main(String[] args){
		Properties props = StringUtils.argsToProperties(args);
		
		ArrayList<Tree> trees = readTrees("dev.1.notraces");
		//ArrayList<Tree> trees = readTrees("/home/lingpenk/Data/PTB/penn_tb_3.0_preprocessed/train.withtop.jack");
	    TreebankLangParserParams params = ReflectionLoading.loadByReflection("edu.stanford.nlp.parser.lexparser.EnglishTreebankParserParams");
	    
	    
	    // generateConll(trees, params);
	    
//		TreeTransformer tt = new TreeBinarizer(params.typedDependencyHeadFinder(), params.treebankLanguagePack(), false, false, 0, false, false, 20.0, false, false, false);
//		TreeTransformer ttt = new TreeBinarizer(new CollinsHeadFinder(), params.treebankLanguagePack(), false, false, 0, false, false, 20.0, false, false, false);
//
//		for (Tree t : trees) {
//			Tree newT = tt.transformTree(t);
//			System.out.println("Original tree:");
//			t.pennPrint();
//			System.out.println("Binarized tree1:");
//			newT.pennPrint();
//			System.out.println();
//			System.out.println("Binarized tree2:");
//			Tree newnewT = ttt.transformTree(t);
//			newnewT.pennPrint();
//			System.out.println();
//			break;
//		}
	    
	    // Head to determine the Head for a tree node
	    LineWriter lw = new LineWriter("rules");
	    generateRules(trees, params, true, lw);
	    lw.closeAll();
	    
		for(Tree t : trees){
			t = trees.get(0);
			GrammaticalStructure gs = params.getGrammaticalStructure(t, Filters.acceptFilter(), params.typedDependencyHeadFinder());
			System.out.println(gs.toString());
			TreeGraphNode tgn = gs.root();
			Tree copy_tree = tgn.deepCopy();
			System.out.println("***" + TreeHelper.getHeadNodeInd(copy_tree));
			System.out.println("***" + TreeHelper.getHeadNodeInd(tgn));
			System.out.println("***" + tgn.label().value());
			((TreeGraphNode)copy_tree).label().set(TreeCoreAnnotations.HeadWordAnnotation.class, new TreeGraphNode());
			//System.out.println("***" + TreeHelper.getHeadNodeInd(copy_tree));
			System.out.println("***" + TreeHelper.getHeadNodeInd(tgn));		
			System.out.println("***" + tgn.pennString());
//			TreeGraphNode newNode = new TreeGraphNode();
//			CoreLabel cl =  new CoreLabel();
//			// This is a generated label
//			cl.setValue(t.label().value() + "|");
//			//cl.set(TreeCoreAnnotations.HeadWordAnnotation.class, (headNode.label().get(TreeCoreAnnotations.HeadWordAnnotation.class)));
//			newNode.setLabel(cl);
//			Tree[] childTrees = new Tree[2];
//			childTrees[0] = tgn.children()[0];
//			childTrees[1] = tgn.children()[0];
//			
//			tgn.setChildren(childTrees);
//			System.out.println("***" + tgn.pennString());
//			System.out.println("***" + newNode.pennString());
//			System.out.println("***" + tgn.children()[0].parent());
//			
////			System.out.println(TreeHelper.getHeadNodeInd(tgn));
////			System.out.println("=======0");
////			System.out.println(tgn.label());
//			for(TreeGraphNode child : tgn.children()){
//				int head_child_int = TreeHelper.headPosition(child, params);
//				System.out.println(head_child_int);
//			}
			TreeHelper.BinarizeTree(tgn, params);
			
//			Stack<Tree> stack = new Stack<Tree>();
//			stack.push(tgn);
//			while(!stack.isEmpty()){
//				TreeGraphNode node = (TreeGraphNode)stack.pop();
//				CoreLabel cl = node.label();
//				cl.setValue(cl.value() + "-" + TreeHelper.getHeadNodeInd(node));
//				for(Tree child : node.children()){
//					stack.push(child);
//				}
//			}
			
			tgn.pennPrint(); 
			
			
			
//			System.out.println("=======1");
//			for(Tree subtree : t.preOrderNodeList()){
//				if(!(subtree.isLeaf() || subtree.isPreTerminal())){
//					System.out.println("=======2");
//					subtree.pennPrint();
//					System.out.println("=======3");
//					Tree headTree = params.typedDependencyHeadFinder().determineHead(subtree);
//					headTree.pennPrint();
//					System.out.println("=======4");
//				}
//			}
			break;
		}
	}
}

