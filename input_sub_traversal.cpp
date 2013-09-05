void anyhow()
{

}

void bar(int a, int b, int c)
{
	anyhow();
	int i;
	anyhow();
}

void foo()
{
	float f;
	bar(42,33,52);
	int i = 42;
}

int main(int argc, char* argv[])
{
	foo(); 
}

