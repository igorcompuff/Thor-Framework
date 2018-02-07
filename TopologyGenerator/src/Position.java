
public class Position 
{
	private double x;
	private double y;
	private double cost;
	
	public Position(double x, double y, double cost)
	{
		this.x = x;
		this.y = y;
		this.cost = cost;
	}
	
	public double getX() {
		return x;
	}
	
	public double getY() {
		return y;
	}
	
	public double getCost() {
		return cost;
	}
	
	public boolean equals(Object obj)
	{
		if (obj instanceof Position)
		{
			Position pos = (Position)obj;
			
			return pos.x == this.x && pos.y == this.y;
		}
		
		
		return false;
	}
}
