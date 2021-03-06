#include "in_scene.h"
#include "mini_xfileLoader.h"
#include "mini_effectLoader.h"
#include "mini_exceptions.h"
#include <sstream>
#include <Windowsx.h>
#include "dinput.h"

using namespace std;
using namespace mini;
using namespace mini::utils;
using namespace DirectX;
bool leftClicked = false;
int oldX = -1;
int oldY = -1;
bool wd = false;
bool sd = false;
bool ad = false;
bool dd = false;
bool ed = false;
bool opened = false;
bool closed = true;

IDirectInput8* di;
IDirectInputDevice8* pMouse;
IDirectInputDevice8* pKeyboard;

const unsigned int GET_STATE_RETRIES = 2;
const unsigned int ACQUIRE_RETRIES = 2;

bool INScene::ProcessMessage(WindowMessage& msg)
{
	/*Process windows messages here*/
	/*if message was processed, return true and set value to msg.result*/
	/*
	switch(msg.message)
	{
	case WM_LBUTTONDOWN:
		if(leftClicked == false)
		{
			leftClicked = true;
			oldY = GET_Y_LPARAM(msg.lParam); 
			oldX = GET_X_LPARAM(msg.lParam);
			
		SetCapture(m_window.getHandle());
		}
		break;

	case WM_LBUTTONUP:
		if(leftClicked == true)
		{
			leftClicked = false;
		ReleaseCapture();
		}
		break;

	case WM_MOUSEMOVE:
		if( leftClicked == true)
		{
			int yPos = GET_Y_LPARAM(msg.lParam); 
			int xPos = GET_X_LPARAM(msg.lParam);

			if (xPos != oldX || yPos != oldY)
			{
				m_camera.Rotate((oldY - yPos) * 0.002, (oldX - xPos) * 0.002);
				oldX = xPos;
				oldY = yPos;
			}
			return true;
		}
		break;

	case WM_KEYDOWN:
		if(msg.wParam == 0x57)
			wd = true;
		 if(msg.wParam == 0x53)
			sd = true;
		 if(msg.wParam == 0x41)
			ad = true;
		 if(msg.wParam == 0x44)
			dd = true;
		 if(msg.wParam == 0x45)
			ed = true;
		break;

	case WM_KEYUP:
		if(msg.wParam == 0x57)
			wd = false;
		 if(msg.wParam == 0x53)
			sd = false;
		 if(msg.wParam == 0x41)
			ad = false;
		 if(msg.wParam == 0x44)
			dd = false;
		 if(msg.wParam == 0x45)
			ed = false;
		break;
	}*/

	return false;
}

void INScene::InitializeInput()
{
	/*Initialize Direct Input resources here*/
	

	HRESULT result = DirectInput8Create(getHandle(),
		DIRECTINPUT_VERSION, IID_IDirectInput8,
		reinterpret_cast<void**>(&di), nullptr);

	
	result = di->CreateDevice(GUID_SysMouse, &pMouse,
		nullptr);

	
	result = di->CreateDevice(GUID_SysKeyboard, &pKeyboard,
		nullptr);

	result = pMouse->SetDataFormat(&c_dfDIMouse);
	result = pKeyboard->SetDataFormat(&c_dfDIKeyboard);

	result = pMouse->SetCooperativeLevel(
		m_window.getHandle(),
		DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);	result = pKeyboard->SetCooperativeLevel(
		m_window.getHandle(),
		DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

	result = pMouse->Acquire();
	result = pKeyboard->Acquire();
}

void INScene::Shutdown()
{
	/*Release Direct Input resources here*/
	
	pMouse->Unacquire();
	pMouse->Release();
	pKeyboard->Unacquire();
	pKeyboard->Release();	di->Release();

	m_font.reset();
	m_fontFactory.reset();
	m_sampler.reset();
	m_texturedEffect.reset();
	m_cbProj.reset();
	m_cbView.reset();
	m_cbModel.reset();
	m_sceneGraph.reset();
	DxApplication::Shutdown();
}

bool GetDeviceState(IDirectInputDevice8* pDevice, unsigned int size, void* ptr)
{
	if (!pDevice)
		return false;
	for (int i = 0; i < GET_STATE_RETRIES; ++i)
	{
		HRESULT result = pDevice->GetDeviceState(size, ptr);
		if (SUCCEEDED(result))
			return true;
		if (result != DIERR_INPUTLOST &&
			result != DIERR_NOTACQUIRED)
		{
			//YOLO1
		}
		for (int j = 0; j < ACQUIRE_RETRIES; ++j)
		{
			result = pDevice->Acquire();
			if (SUCCEEDED(result))
				break;
			if (result != DIERR_INPUTLOST &&
				result != E_ACCESSDENIED)
			{
				//yolo2
			}
		}
	}
	return false;
}


void INScene::Update(float dt)
{
	/*proccess Direct Input here*/
	
	/*remove the following line to stop the initial camera animation*/
	//m_camera.Rotate(0.0f, dt*XM_PIDIV4);
	leftClicked = false;
	wd = false;
	sd = false;
	ad = false;
	dd = false;
	ed = false;

	DIMOUSESTATE state;
	if (GetDeviceState(pMouse, sizeof(DIMOUSESTATE), &state))
	{
		if (state.rgbButtons[0])
		{
			if (leftClicked == false)
				leftClicked = true;
		}

		if (leftClicked == true)
		{
			if (oldX == -1)
				oldX = state.lX;
			if (oldY == -1)
				oldY = state.lY;

			if (state.lX != oldX || state.lY != oldY)
			{
				m_camera.Rotate((oldY + state.lY) * 0.002, (oldX + state.lX) * 0.002);
				oldX = state.lX;
				oldY = state.lY;
			}
		}
	}	BYTE states[256];
	if (GetDeviceState(pKeyboard, sizeof(BYTE) * 256, states))
	{
		if (states[DIK_W])
			wd = true;
		if (states[DIK_S])
			sd = true; 
		if (states[DIK_A])
			ad = true;
		if (states[DIK_D])
			dd = true;
		if (states[DIK_E])
			ed = true;		
	}


	if(wd == true)
		MoveCharacter(0,1 * dt);
	
	if(sd == true)
		MoveCharacter(0,-1 * dt);
	
	if(ad == true)
		MoveCharacter(-1 * dt,0);
	
	if(dd == true)
		MoveCharacter(1 * dt,0);

	if(ed == true)
	{
		if(FacingDoor() == true && DistanceToDoor() <= 1.0f )
		{
			if(opened == false && closed == true)
			{
				OpenDoor();
				opened = true;
				closed = false;
			}
			
			else if(opened == true && closed == false)
			{
				CloseDoor();
				opened = false;
				closed = true;
			}
			
		}
	}

	m_counter.NextFrame(dt);
	UpdateDoor(dt);
}

void INScene::RenderText()
{
	wstringstream str;
	str << L"FPS: " << m_counter.getCount();
	m_font->DrawString(m_context.get(), str.str().c_str(), 20.0f, 10.0f, 10.0f, 0xff0099ff, FW1_RESTORESTATE|FW1_NOGEOMETRYSHADER);
	if (DistanceToDoor() < 1.0f && FacingDoor())
	{
		wstring prompt(L"(E) Otw�rz/Zamknij");
		FW1_RECTF layout;
		auto rect = m_font->MeasureString(prompt.c_str(), L"Calibri", 20.0f, &layout, FW1_NOWORDWRAP);
		float width = rect.Right - rect.Left;
		float height = rect.Bottom - rect.Top;
		auto clSize = m_window.getClientSize();
		m_font->DrawString(m_context.get(), prompt.c_str(), 20.0f, (clSize.cx - width)/2, (clSize.cy - height)/2, 0xff00ff99,  FW1_RESTORESTATE|FW1_NOGEOMETRYSHADER);
	}
}

bool INScene::Initialize()
{
	if (!DxApplication::Initialize())
		return false;
	XFileLoader xloader(m_device);
	xloader.Load("house.x");
	m_sceneGraph.reset(new SceneGraph(move(xloader.m_nodes), move(xloader.m_meshes), move(xloader.m_materials)));

	m_doorNode = m_sceneGraph->nodeByName("Door");
	m_doorTransform = m_sceneGraph->getNodeTransform(m_doorNode);
	m_doorAngle = 0;
	m_doorAngVel = -XM_PIDIV2;

	m_cbProj.reset(new ConstantBuffer<XMFLOAT4X4>(m_device));
	m_cbView.reset(new ConstantBuffer<XMFLOAT4X4>(m_device));
	m_cbModel.reset(new ConstantBuffer<XMFLOAT4X4, 2>(m_device));
	m_cbMaterial.reset(new ConstantBuffer<Material::MaterialData>(m_device));

	EffectLoader eloader(m_device);
	eloader.Load(L"textured.hlsl");
	m_texturedEffect.reset(new TexturedEffect(move(eloader.m_vs), move(eloader.m_ps), m_cbProj, m_cbView, m_cbModel, m_cbMaterial));
	D3D11_INPUT_ELEMENT_DESC elem[] = 
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	m_layout = m_device.CreateInputLayout(elem, 3, eloader.m_vsCode);

	SIZE s = m_window.getClientSize();
	float ar = static_cast<float>(s.cx) / s.cy;
	XMStoreFloat4x4(&m_proj, XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), ar, 0.01f, 100.0f));
	m_cbProj->Update(m_context, m_proj);

	vector<OrientedBoundingRectangle> obstacles;
	obstacles.push_back(OrientedBoundingRectangle(XMFLOAT2(-3.0f, 3.8f), 6.0f, 0.2f, 0.0f));
	obstacles.push_back(OrientedBoundingRectangle(XMFLOAT2(-3.0f, -4.0f), 6.0f, 0.2f, 0.0f));
	obstacles.push_back(OrientedBoundingRectangle(XMFLOAT2(2.8f, -3.8f), 0.2f, 7.6f, 0.0f));
	obstacles.push_back(OrientedBoundingRectangle(XMFLOAT2(-3.0f, -3.8f), 0.2f, 4.85f, 0.0f));
	obstacles.push_back(OrientedBoundingRectangle(XMFLOAT2(-3.0f, 1.95f), 0.2f, 1.85f, 0.0f));
	obstacles.push_back(OrientedBoundingRectangle(XMFLOAT2(-3.05f, 1.0f), 0.1f, 1.0f, 0.0f));
	m_collisions.SetObstacles(move(obstacles));

	RasterizerDescription rsDesc;
	m_rsState = m_device.CreateRasterizerState(rsDesc);
	SamplerDescription sDesc;
	m_sampler = m_device.CreateSamplerState(sDesc);
	ID3D11SamplerState* sstates[1] = { m_sampler.get() };
	m_context->PSSetSamplers(0, 1, sstates);
	
	IFW1Factory *pf;
	HRESULT result = FW1CreateFactory(FW1_VERSION, &pf);
	m_fontFactory.reset(pf);
	if (FAILED(result))
		THROW_DX11(result);
	IFW1FontWrapper *pfw;
	result = m_fontFactory->CreateFontWrapper(m_device.getDevicePtr(), L"Calibri", &pfw);
	m_font.reset(pfw);
	if (FAILED(result))
		THROW_DX11(result);

	InitializeInput();
	return true;
}

void INScene::Render()
{
	if (!m_context)
		return;
	DxApplication::Render();
	m_context->RSSetState(m_rsState.get());
	XMFLOAT4X4 mtx[2];
	XMStoreFloat4x4(&mtx[0], m_camera.GetViewMatrix());
	m_cbView->Update(m_context, mtx[0]);
	m_texturedEffect->Begin(m_context);
	for ( unsigned int i = 0; i < m_sceneGraph->meshCount(); ++i)
	{
		Mesh& m = m_sceneGraph->getMesh(i);
		Material& material = m_sceneGraph->getMaterial(m.getMaterialIdx());
		if (!material.getDiffuseTexture())
			continue;
		ID3D11ShaderResourceView* srv[2] = { material.getDiffuseTexture().get(), material.getSpecularTexture().get() };
		m_context->PSSetShaderResources(0, 2, srv);
		mtx[0] = m.getTransform();
		XMMATRIX modelit = XMLoadFloat4x4(&mtx[0]);
		XMVECTOR det;
		modelit = XMMatrixTranspose(XMMatrixInverse(&det, modelit));
		XMStoreFloat4x4(&mtx[1], modelit);
		m_cbMaterial->Update(m_context, material.getMaterialData());
		m_cbModel->Update(m_context, mtx);
		m_context->IASetInputLayout(m_layout.get());
		m.Render(m_context);
	}
	RenderText();
}

void INScene::OpenDoor()
{
	if (m_doorAngVel < 0)
		m_doorAngVel = -m_doorAngVel;
}

void INScene::CloseDoor()
{
	if (m_doorAngVel > 0)
		m_doorAngVel = -m_doorAngVel;
}

void INScene::ToggleDoor()
{
	m_doorAngVel = -m_doorAngVel;
}

void INScene::UpdateDoor(float dt)
{
	if ((m_doorAngVel > 0 && m_doorAngle < XM_PIDIV2) || (m_doorAngVel < 0 && m_doorAngle > 0))
	{
		m_doorAngle += dt*m_doorAngVel;
		if ( m_doorAngle < 0 )
			m_doorAngle = 0;
		else if (m_doorAngle > XM_PIDIV2)
			m_doorAngle = XM_PIDIV2;
		XMFLOAT4X4 doorTransform;
		XMMATRIX mtx = XMLoadFloat4x4(&m_doorTransform);
		XMVECTOR v = XMVectorSet(0.000004f, 0.0f, -1.08108f, 1.0f);
		v = XMVector3TransformCoord(v, mtx);
		XMStoreFloat4x4(&doorTransform, mtx*XMMatrixTranslationFromVector(-v)*XMMatrixRotationZ(m_doorAngle)*XMMatrixTranslationFromVector(v));
		m_sceneGraph->setNodeTransform(m_doorNode, doorTransform);
		XMFLOAT2 tr = m_collisions.MoveObstacle(5, OrientedBoundingRectangle(XMFLOAT2(-3.05f, 1.0f), 0.1f, 1.0f, m_doorAngle));
		m_camera.Move(XMFLOAT3(tr.x, 0, tr.y));
	}
}

void INScene::MoveCharacter(float dx, float dz)
{
	XMVECTOR forward = m_camera.getForwardDir();
	XMVECTOR right = m_camera.getRightDir();
	XMFLOAT3 temp;
	XMStoreFloat3(&temp, forward*dz + right*dx);
	XMFLOAT2 tr = XMFLOAT2(temp.x, temp.z);
	m_collisions.MoveCharacter(tr);
	m_camera.Move(XMFLOAT3(tr.x, 0, tr.y));
}

bool INScene::FacingDoor()
{
	auto rect = m_collisions.getObstacle(5);
	XMVECTOR points[4] = { XMLoadFloat2(&rect.getP1()), XMLoadFloat2(&rect.getP2()), XMLoadFloat2(&rect.getP3()), XMLoadFloat2(&rect.getP4()) };
	XMVECTOR forward = XMVectorSwizzle(m_camera.getForwardDir(), 0, 2, 1, 3);
	XMVECTOR camera = XMVectorSwizzle(XMLoadFloat4(&m_camera.getPosition()), 0, 2, 1, 3);
	unsigned int max_i = 0, max_j = 0;
	float max_a = 0.0f;
	for (unsigned int i = 0; i < 4; ++i)
	{
		for (unsigned int j = i + 1; j < 4; ++j)
		{
			float a = XMVector2AngleBetweenVectors(points[i]-camera, points[j]-camera).m128_f32[0];
			if (a > max_a)
			{
				max_i = i;
				max_j = j;
				max_a = a;
			}
		}
	}
	return XMScalarNearEqual(XMVector2AngleBetweenVectors(forward, points[max_i]-camera).m128_f32[0] + XMVector2AngleBetweenVectors(forward, points[max_j] - camera).m128_f32[0], max_a, 0.001f);
}

float INScene::DistanceToDoor()
{
	return m_collisions.DistanceToObstacle(5);
}