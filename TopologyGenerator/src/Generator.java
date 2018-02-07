import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.LinkedList;
import java.util.List;
import java.util.Random;
import java.util.stream.Collectors;

public class Generator 
{
	private String positionsBaseDir;
	private double badNodeProbability;
	private List<Position> positionList;
	private int maxRedundancy;
	private int replicas;
	private Random rnd;
	
	public Generator(String positionsBaseDir, double badNodeProbability, int replicas, int maxRedundancy)
	{
		this.positionsBaseDir = positionsBaseDir;
		this.badNodeProbability = badNodeProbability;
		this.maxRedundancy = maxRedundancy;
		this.replicas = replicas;
		
		positionList = new LinkedList<>();
		rnd = new Random();
		
	}
	
	public void BuildPositionList() throws IOException
	{
		List<Path> files = Files.list(Paths.get(positionsBaseDir)).collect(Collectors.toList());
		
		for(Path file : files)
		{
			List<String> lines = Files.lines(file).collect(Collectors.toList());
			
			double x = Double.parseDouble(lines.get(0).split(" = ")[1]);
			double y = Double.parseDouble(lines.get(1).split(" = ")[1]);
			double cost = Double.parseDouble(lines.get(2).split(" = ")[1]);
			
			positionList.add(new Position(x, y, cost));
			
		}
	}
	
	public void CreateSequentialTopology(BadNodeCriterion badCriterion, PositionSelectionCriterion selectionCriterion) throws IOException
	{
		BuildPositionList();
		
		Path baseDir = Paths.get("/home/igor/github/ns-3.26/redundancytest/");
		
		List<Position> setectedList;
		
		for(int i = 1; i <= replicas; i++)
		{
			setectedList = select(maxRedundancy, badCriterion, selectionCriterion);
			
			for(int redundancy = 1; redundancy <= maxRedundancy; redundancy++)
			{
				Path redDir = baseDir.resolve(Paths.get(redundancy + "/"));
				
				if (!Files.exists(redDir))
				{
					Files.createDirectories(redDir);
				}
				
				Path filePath = redDir.resolve(Paths.get(i + ".txt"));
				Files.createFile(filePath);
				LinkedList<String> lines = new LinkedList<>();
				for (int j = 0; j < redundancy; j++)
				{
					lines.add("X = " + setectedList.get(j).getX());
					lines.add("Y = " + setectedList.get(j).getY());
					lines.add("Cost = " + setectedList.get(j).getCost());
					lines.add("########################");
				}
				
				Files.write(filePath, lines);
			}
		}
		
	}
	
	public void CreateRandomTopology(int redundancy, BadNodeCriterion badCriterion, PositionSelectionCriterion selectionCriterion) throws IOException
	{
		BuildPositionList();
		
		Path dir = Paths.get("/home/igor/github/ns-3.26/topologies/" + redundancy );
		if (!Files.exists(dir))
		{
			Files.createDirectories(dir);
		}
		
		for(int i = 1; i <= replicas; i++)
		{
			LinkedList<String> lines = new LinkedList<>();
			List<Position> selectedList = select(redundancy, badCriterion, selectionCriterion);
			
			for(Position selected : selectedList)
			{
				
				lines.add("X = " + selected.getX());
				lines.add("Y = " + selected.getY());
				lines.add("Cost = " + selected.getCost());
				lines.add("########################");
				
			}
			
			Path filePath = dir.resolve(i + ".txt");
			Files.createFile(filePath);
			Files.write(filePath, lines);
		}
	}
	
	private List<Position> select(int total, BadNodeCriterion badCriterion, PositionSelectionCriterion selectionCriterion)
	{
		LinkedList<Position> selectedList = new LinkedList<>();
		
		for (int i = 1; i <= total; i++)
		{
			Position selected = null;
			
			while (selected == null)
			{
				int index = rnd.nextInt(positionList.size());
				Position pos = positionList.get(index);
				int rand = rnd.nextInt(100);
				
				boolean selectable = selectionCriterion.selectable(pos) && !selectedList.contains(pos);
				
				if (selectable)
				{
					if (rand < 100 * badNodeProbability)
					{
						if (badCriterion.isBad(pos))
						{
							selected = pos;
						}
					}
					else
					{
						selected = pos;
					}
				}
			}
			
			selectedList.add(selected);
		}
		
		
		return selectedList;
	}
	
	public static void main(String[] args)
	{	
		BadNodeCriterion badCrit = new BadNodeCriterion() {
			
			@Override
			public boolean isBad(Position pos) 
			{
				return pos.getCost() >= 5.0;
			}
		};
		
		PositionSelectionCriterion selectionCrit = new PositionSelectionCriterion() {
			
			@Override
			public boolean selectable(Position pos) {
				// TODO Auto-generated method stub
				return true;
			}
		};
		
		
		try 
		{
			Generator gen = new Generator("/home/igor/github/ns-3.26/positions", 0.3, 20, 10);
			/*for (int i = 1; i <= maxRedundancy; i++)
			{
				gen.CreateRandomTopology(i, badCrit, selectionCrit);
			}*/
			
			gen.CreateSequentialTopology(badCrit, selectionCrit);
			
			System.out.println("Done");
		} 
		catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
}
