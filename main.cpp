#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <algorithm>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/animator.h>
#include <learnopengl/model_animation.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "Player.h"
#include "HumanItem.h"
#include <mass.h>
#include "SpinItem.h"

// FUNCTION PROTOTYPES
GLFWwindow *glAllInit();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
//void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void ModelLoading();


// Custom Functions

// Set Initial Particle Position
void particleInit(int index);   

// Running Particle Animation
void UpdateParticleAnim();

// Play Particle Animation at Index
void PlayParticleAtIndex(int index);


// GLOBAL VARIABLES

// All GameObjects
vector<GameObject*> GameObjects;
// Preloading
vector<HumanItem*> ProfessorPool;
vector<HumanItem*> GirlFriendPool;
vector<SpinItem*> BookPool;
vector<SpinItem*> GamepadPool;
vector<SpinItem*> CalculatorPool;
vector<SpinItem*> BeerPool;

// Eat Particles
vector<vector<Mass*>> ParticleVector;
float initialParticlePosY = 0.f;
float initialParticlePosX = -2.0f;
float particlePosXOffset = 2.0f;
float particleMass = 1.f;
Shader* particleShader = NULL;
float deltaT = 1.0f / 30.0f;
float timeT[] = { 0.f, 0.f, 0.f };
int nFrame[] = { 0, 0, 0 };
bool bShowParticle[] = {false, false, false };

// Source and Data directories
string sourceDirStr = "E:/Setup_Windows/SkeletalAnimation/SkeletalAnimation";
string modelDirStr = "E:/Setup_Windows/data";

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
GLFWwindow *mainWindow = NULL;

// Camera Position
glm::vec3 CameraPosition(0.0f, 4.0f, 5.0f);
glm::vec3 CameraTargetPosition(0.0f, 1.6f, 0.0f);
int ZoomAmount = 0;
glm::vec3 TargetOffset(0.0f, 0.0f, 0.0f);
glm::vec3 CameraOffset(0.0f, -1.0f, 0.0f);

// camera
Camera camera(CameraPosition);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Input Models
Player* player;
string human_vs = sourceDirStr + "/human.vs";
string human_fs = sourceDirStr + "/human.fs";
string particle_vs = sourceDirStr + "/particle.vs";
string particle_fs = sourceDirStr + "/particle.fs";
string item_vs = sourceDirStr + "/spinitem.vs";
string item_fs = sourceDirStr + "/spinitem.fs";
string playerModelPath = modelDirStr + "/FastRun/FastRun.dae";
string professorModelPath = modelDirStr + "/Professor/Professor.dae";
string girlfiendModelPath = modelDirStr + "/girlfriend/girlfriend.dae";
string BeerModelPath = modelDirStr + "/Beer/Beer.dae";
string CalculatorModelPath = modelDirStr + "/Calculator/Calculator.dae";
string GamepadModelPath = modelDirStr + "/Gamepad/Gamepad.dae";
string BookModelPath = modelDirStr + "/Book/Book.dae";

int main()
{
    mainWindow = glAllInit();
	glfwSetKeyCallback(mainWindow, key_callback);

	// draw in wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// Model Preloading 
	ModelLoading();

	// Initialize Particles
	for (int i = 0; i < 3; i++)
	{
		vector<Mass*> newVector;
		for (int i = 0; i < 8; i++)
		{
			newVector.push_back(new Mass(particleMass));
			newVector[i]->setPosition(0, -50, 0);
			newVector[i]->setVelocity(0.0, 0.0, 0.0);
			newVector[i]->setAcceleration(0.0, 0.0, 0.0);
		}
		ParticleVector.push_back(newVector);
	}
	particleShader = new Shader(particle_vs.c_str(), particle_fs.c_str());

	// render loop
	// -----------
	while (!glfwWindowShouldClose(mainWindow))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		camera.Position = CameraPosition;
		camera.Front = CameraTargetPosition - CameraPosition;

		// render
		// ------
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// view/projection transformations
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();

		// render the loaded model
		// Update, Render All GameObjects
		for (GameObject* gameObject : GameObjects)
		{
			if (!gameObject->bActivated) continue;

			gameObject->Update(deltaTime);
			gameObject->Render(projection, view);

			// If not Player, collision check with player
			if (gameObject->bIsItem)
			{
				Item* item = (Item*)gameObject;
				if (item->IsCollideWithPlayer(*player) )
				{
					PlayParticleAtIndex(item->GetLineIndex());
					item->CollisionEvent();
					item->SetInitialPosition(-2.f + particlePosXOffset * item->GetLineIndex(), -10);
				}
				else if (item->_transform->getLocalPosition().z >= 3)
				{
					item->bActivated = false;
					item->SetInitialPosition(-2.f + particlePosXOffset * item->GetLineIndex(), -10);
				}
			}
		}

		// Running Particle Animation
		UpdateParticleAnim();

		// Particle Rendering
		particleShader->use();
		particleShader->setMat4("projection", projection);
		particleShader->setMat4("view", view);
		glm::mat4 particleModel = glm::mat4(1.0);
		particleShader->setMat4("model", particleModel);
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				ParticleVector[i][j]->draw(particleShader, 1.0f, 1.0f, 1.0f);
			}
		}

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(mainWindow);
		glfwPollEvents();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

GLFWwindow *glAllInit()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Campus Run", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(-1);
    }
    
    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);
    
    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    return window;
}

void ModelLoading()
{
	// Initialize Player
	player = new Player();
	GameObjects.push_back((GameObject*)player);
	player->SetShader(human_vs, human_fs);
	player->SetModel(playerModelPath);
	player->bIsItem = false;

	// Preload Professor and Girlfriend
	for (int i = 0; i < 3; i++)
	{
		HumanItem* newHuman = new HumanItem(professorModelPath, 0, ScoreOfItem[ItemType::Professor], glm::vec3(0.8f, 0.8f, 0.8f));
		newHuman->SetInitialPosition(-2, -10);
		newHuman->SetShader(human_vs, human_fs);
		newHuman->bActivated = false;
		newHuman->SetCollisionBound(.5f, 2.f, .5f);
		ProfessorPool.push_back(newHuman);

		HumanItem* newHuman2 = new HumanItem(girlfiendModelPath, 0, ScoreOfItem[ItemType::GirlFriend], glm::vec3(0.8f, 0.8f, 0.8f));
		newHuman2->SetInitialPosition(-2, -10);
		newHuman2->SetShader(human_vs, human_fs);
		newHuman2->bActivated = false;
		newHuman2->SetCollisionBound(.5f, 2.f, .5f);
		GirlFriendPool.push_back(newHuman2);
	}

	// Preload Spin Item
	for (int i = 0; i < 4; i++)
	{
		SpinItem* newItem = new SpinItem(BeerModelPath, 0, ScoreOfItem[ItemType::Beer], glm::vec3(4.f, 4.f, 4.f));
		newItem->SetInitialPosition(-2, -10);
		newItem->SetShader(item_vs, item_fs);
		newItem->bActivated = false;
		newItem->SetCollisionBound(.5f, 1.f, .5f);
		BeerPool.push_back(newItem);
	}
	for (int i = 0; i < 4; i++)
	{
		SpinItem* newItem = new SpinItem(BookModelPath, 0, ScoreOfItem[ItemType::Book], glm::vec3(0.3f, 0.3f, 0.3f));
		newItem->SetInitialPosition(-2, -10);
		newItem->SetShader(item_vs, item_fs);
		newItem->bActivated = false;
		newItem->SetCollisionBound(.5f, 1.f, .5f);
		BookPool.push_back(newItem);
	}
	for (int i = 0; i < 4; i++)
	{
		SpinItem* newItem = new SpinItem(CalculatorModelPath, 0, ScoreOfItem[ItemType::Calculator], glm::vec3(6.f, 6.f, 6.f));
		newItem->SetInitialPosition(-2, -10);
		newItem->SetShader(item_vs, item_fs);
		newItem->bActivated = false;
		newItem->SetCollisionBound(.5f, 1.f, .5f);
		CalculatorPool.push_back(newItem);
	}
	for (int i = 0; i < 4; i++)
	{
		SpinItem* newItem = new SpinItem(GamepadModelPath, 0, ScoreOfItem[ItemType::GamePad], glm::vec3(0.05f, 0.05f, 0.05f));
		newItem->SetInitialPosition(-2, -10);
		newItem->SetShader(item_vs, item_fs);
		newItem->bActivated = false;
		newItem->SetCollisionBound(.5f, 1.f, .5f);
		GamepadPool.push_back(newItem);
	}

}

void UpdateParticleAnim()
{
	for (int i = 0; i < 3; i++)
	{
		// If Current Index Particle Animation is Playing
		if (bShowParticle[i])
		{
			// If Animation Started
			if (nFrame[i] == 0)
			{
				for (int j = 0; j < 8; j++)
				{
					ParticleVector[i][j]->euler(timeT[i], deltaT,  30 * cos((2 * 3.14 / 8) * j), 100, 30 * sin((2 * 3.14 / 8) * j));
				}
			}
			// Continue
			else
			{
				for (int j = 0; j < 8; j++)
				{
					ParticleVector[i][j]->euler(timeT[i], deltaT, 0.0, 0.0, 0.0);
				}

			}

			timeT[i] = timeT[i] + deltaT;
			nFrame[i]++;

			// When Particles reach under ground, animation has to be stopped
			if (ParticleVector[i][0]->p[1] < -5.f)
			{
				for (int j = 0; j < 8; j++)
				{
					bShowParticle[i] = false;
				}
			}
		}
	}
}

void particleInit(int index) {

	for (int i = 0; i < 8; i++)
	{
		ParticleVector[index][i]->setPosition(initialParticlePosX + index * particlePosXOffset, player->_transform->getLocalPosition().y, 0.0);
		ParticleVector[index][i]->setVelocity(0.0, 0.0, 0.0);
		ParticleVector[index][i]->setAcceleration(0.0, 0.0, 0.0);
	}
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	for (GameObject* gameObject : GameObjects)
	{
		gameObject->RunKeyCallback(window, key, scancode, action, mods);
	}

	if (key == GLFW_KEY_UP && action == GLFW_PRESS)
	{
		if (ZoomAmount == 2) return;

		ZoomAmount++;
		CameraTargetPosition += TargetOffset;
		CameraPosition += CameraOffset;
	}

	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
	{
		if (ZoomAmount == 0) return;

		ZoomAmount--;
		CameraTargetPosition -= TargetOffset;
		CameraPosition -= CameraOffset;
	}

	// Human Item Animation Test
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
	{
		for (int i = 0; i < ProfessorPool.size(); i++)
		{
			if (ProfessorPool[i]->bActivated == false)
			{
				if (!(find(GameObjects.begin(), GameObjects.end(), ProfessorPool[i]) != GameObjects.end()))
				{
					GameObjects.push_back((GameObject*)ProfessorPool[i]);
				}
				ProfessorPool[i]->bActivated = true;
				ProfessorPool[i]->SetInitialPosition(-2,  -10.f);
				break;
			}
		}
	}

	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
	{
		for (int i = 0; i < GirlFriendPool.size(); i++)
		{
			if (GirlFriendPool[i]->bActivated == false)
			{
				if (!(find(GameObjects.begin(), GameObjects.end(), GirlFriendPool[i]) != GameObjects.end()))
				{
					GameObjects.push_back((GameObject*)GirlFriendPool[i]);
				}
				GirlFriendPool[i]->bActivated = true;
				GirlFriendPool[i]->SetInitialPosition(-2, -10.f);
				break;
			}
		}
	}
	if (key == GLFW_KEY_3 && action == GLFW_PRESS)
	{
		for (int i = 0; i < BookPool.size(); i++)
		{
			if (BookPool[i]->bActivated == false)
			{
				if (!(find(GameObjects.begin(), GameObjects.end(), BookPool[i]) != GameObjects.end()))
				{
					GameObjects.push_back((GameObject*)BookPool[i]);
				}
				BookPool[i]->bActivated = true;
				BookPool[i]->SetInitialPosition(-2, -10.f);
				BookPool[i]->SetAnimInit();
				break;
			}
		}
	}
	if (key == GLFW_KEY_4 && action == GLFW_PRESS)
	{
		for (int i = 0; i < GamepadPool.size(); i++)
		{
			if (GamepadPool[i]->bActivated == false)
			{
				if (!(find(GameObjects.begin(), GameObjects.end(), GamepadPool[i]) != GameObjects.end()))
				{
					GameObjects.push_back((GameObject*)GamepadPool[i]);
				}
				GamepadPool[i]->bActivated = true;
				GamepadPool[i]->SetInitialPosition(-2, -10.f);
				GamepadPool[i]->SetAnimInit();
				break;
			}
		}
	}
	if (key == GLFW_KEY_5 && action == GLFW_PRESS)
	{
		for (int i = 0; i < CalculatorPool.size(); i++)
		{
			if (CalculatorPool[i]->bActivated == false)
			{
				if (!(find(GameObjects.begin(), GameObjects.end(), CalculatorPool[i]) != GameObjects.end()))
				{
					GameObjects.push_back((GameObject*)CalculatorPool[i]);
				}
				CalculatorPool[i]->bActivated = true;
				CalculatorPool[i]->SetInitialPosition(-2, -10.f);
				CalculatorPool[i]->SetAnimInit();
				break;
			}
		}
	}
	if (key == GLFW_KEY_6 && action == GLFW_PRESS)
	{
		for (int i = 0; i < BeerPool.size(); i++)
		{
			if (BeerPool[i]->bActivated == false)
			{
				if (!(find(GameObjects.begin(), GameObjects.end(), BeerPool[i]) != GameObjects.end()))
				{
					GameObjects.push_back((GameObject*)BeerPool[i]);
				}
				BeerPool[i]->bActivated = true;
				BeerPool[i]->SetInitialPosition(-2, -10.f);
				BeerPool[i]->SetAnimInit();
				break;
			}
		}
	}
}

void PlayParticleAtIndex(int index)
{
	particleInit(index);
	nFrame[index] = 0;
	timeT[index] = 0;
	bShowParticle[index] = true;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}
