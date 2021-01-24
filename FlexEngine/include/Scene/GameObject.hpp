#pragma once

IGNORE_WARNINGS_PUSH
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
IGNORE_WARNINGS_POP

#include "Audio/AudioCue.hpp"
#include "Callbacks/InputCallbacks.hpp"
#include "Graphics/RendererTypes.hpp"
#include "Graphics/VertexBufferData.hpp" // For VertexBufferDataCreateInfo
#include "Helpers.hpp"
#include "Spring.hpp"
#include "Transform.hpp"
#include "Time.hpp"

class btCollisionShape;

namespace flex
{
	class BaseScene;
	class BezierCurveList;
	class Mesh;
	class MeshComponent;
	class Socket;
	class TerminalCamera;
	class Wire;
	class SoftBody;

	namespace VM
	{
		class VirtualMachine;
	}

	struct ChildIndex
	{
		ChildIndex(const std::list<u32>& siblingIndices) :
			siblingIndices(siblingIndices)
		{}

		void Add(u32 siblingIndex)
		{
			siblingIndices.emplace_back(siblingIndex);
		}

		u32 Pop()
		{
			u32 result = siblingIndices.front();
			siblingIndices.pop_front();
			return result;
		}

		bool IsValid()
		{
			return !siblingIndices.empty();
		}

		// Stores sibling index for each level down the hierarchy to reach this child
		// (0th index = index under root, 1st = index under first child, etc.)
		std::list<u32> siblingIndices;
	};

	extern ChildIndex InvalidChildIndex;

	class GameObject
	{
	public:
		enum CopyFlags : u32
		{
			CHILDREN				= (1 << 0),
			MESH					= (1 << 1),
			RIGIDBODY				= (1 << 2),
			ADD_TO_SCENE			= (1 << 3),
			CREATE_RENDER_OBJECT	= (1 << 4),

			ALL = 0xFFFFFFFF,
			_NONE = 0
		};

		GameObject(const std::string& name, StringID typeID, const GameObjectID& gameObjectID = InvalidGameObjectID);
		virtual ~GameObject();

		static GameObject* CreateObjectFromJSON(
			const JSONObject& obj,
			BaseScene* scene,
			i32 sceneFileVersion,
			bool bIsPrefabTemplate = false,
			CopyFlags copyFlags = CopyFlags::ALL);

		static GameObject* CreateObjectFromPrefabTemplate(
			const PrefabID& prefabID,
			std::string& objectName,
			const GameObjectID& gameObjectID,
			GameObject* parent = nullptr,
			Transform* optionalTransform = nullptr,
			CopyFlags copyFlags = CopyFlags::ALL);

		static GameObject* CreateObjectOfType(StringID typeID, const std::string& objectName, const GameObjectID& gameObjectID = InvalidGameObjectID, const char* optionalTypeStr = nullptr);

		// Returns a new game object which is a direct copy of this object, parented to parent
		// If parent == nullptr then new object will have same parent as this object
		virtual GameObject* CopySelf(
			GameObject* parent = nullptr,
			CopyFlags copyFlags = CopyFlags::ALL,
			std::string* optionalName = nullptr,
			const GameObjectID& optionalGameObjectID = InvalidGameObjectID);

		virtual void Initialize();
		virtual void PostInitialize();
		virtual void Destroy(bool bDetachFromParent = true);
		virtual void Update();

		virtual void DrawImGuiObjects();

		virtual void OnTransformChanged();

		virtual void ParseJSON(
			const JSONObject& obj,
			BaseScene* scene,
			i32 fileVersion,
			MaterialID overriddenMatID = InvalidMaterialID,
			bool bIsPrefabTemplate = false,
			CopyFlags copyFlags = CopyFlags::ALL);

		virtual bool AllowInteractionWith(GameObject* gameObject);
		virtual void SetInteractingWith(GameObject* gameObject);

		bool IsBeingInteractedWith() const;

		// Returns true if this object was deleted or duplicated
		bool DoImGuiContextMenu(bool bActive);

		Transform* GetTransform();
		const Transform* GetTransform() const;

		bool DrawImGuiDuplicateGameObjectButton();

		GameObject* GetObjectInteractingWith();

		JSONObject Serialize(const BaseScene* scene, bool bSerializePrefabData = false) const;

		void RemoveRigidBody();

		void SetParent(GameObject* parent);
		GameObject* GetParent();
		void DetachFromParent();

		// Returns a list of objects, starting with the root, going up to this object
		std::vector<GameObject*> GetParentChain();

		// Walks up the tree to the highest parent
		GameObject* GetRootParent();

		GameObject* AddChild(GameObject* child);
		GameObject* AddChildImmediate(GameObject* child);
		bool RemoveChildImmediate(GameObjectID childID, bool bDestroy);
		bool RemoveChildImmediate(GameObject* child, bool bDestroy);
		const std::vector<GameObject*>& GetChildren() const;
		u32 GetChildCountOfType(StringID objTypeID, bool bRecurse);

		GameObject* AddSibling(GameObject* child);
		GameObject* AddSiblingImmediate(GameObject* child);

		template<class T>
		void GetChildrenOfType(StringID objTypeID, bool bRecurse, std::vector<T*>& children)
		{
			if (m_TypeID == objTypeID)
			{
				children.push_back((T*)this);
			}

			if (bRecurse)
			{
				for (GameObject* child : m_Children)
				{
					child->GetChildrenOfType(objTypeID, bRecurse, children);
				}
			}
		}


		bool HasChild(GameObject* child, bool bCheckChildrensChildren);

		void UpdateSiblingIndices(i32 myIndex);
		i32 GetSiblingIndex() const;

		// Returns all objects who share our parent
		std::vector<GameObject*> GetAllSiblings();
		// Returns all objects who share our parent and have a larger sibling index
		std::vector<GameObject*> GetEarlierSiblings();
		// Returns all objects who share our parent and have a smaller sibling index
		std::vector<GameObject*> GetLaterSiblings();

		void AddTag(const std::string& tag);
		bool HasTag(const std::string& tag);
		std::vector<std::string> GetTags() const;

		std::string GetName() const;
		void SetName(const std::string& newName);

		bool IsSerializable() const;
		void SetSerializable(bool bSerializable);

		bool IsStatic() const;
		void SetStatic(bool bStatic);

		bool IsVisible() const;
		virtual void SetVisible(bool bVisible, bool bEffectChildren = true);

		// If bIncludingChildren is true, true will be returned if this or any children are visible in scene explorer
		bool IsVisibleInSceneExplorer(bool bIncludingChildren = false) const;
		void SetVisibleInSceneExplorer(bool bVisibleInSceneExplorer);

		bool HasUniformScale() const;
		void SetUseUniformScale(bool bUseUniformScale, bool bEnforceImmediately);

		btCollisionShape* SetCollisionShape(btCollisionShape* collisionShape);
		btCollisionShape* GetCollisionShape() const;

		RigidBody* SetRigidBody(RigidBody* rigidBody);
		RigidBody* GetRigidBody() const;

		Mesh* GetMesh();
		Mesh* SetMesh(Mesh* mesh);

		bool CastsShadow() const;
		void SetCastsShadow(bool bCastsShadow);

		// Called when another object has begun to overlap
		void OnOverlapBegin(GameObject* other);
		// Called when another object is no longer overlapping
		void OnOverlapEnd(GameObject* other);

		StringID GetTypeID() const;

		void AddSelfAndChildrenToVec(std::vector<GameObject*>& vec);
		void RemoveSelfAndChildrenToVec(std::vector<GameObject*>& vec);

		void AddSelfIDAndChildrenToVec(std::vector<GameObjectID>& vec);
		void RemoveSelfIDAndChildrenToVec(std::vector<GameObjectID>& vec);

		bool SelfOrChildIsSelected() const;

		void SetNearbyInteractable(GameObject* nearbyInteractable);

		bool IsTemplate() const;

		ChildIndex ComputeChildIndex() const;
		ChildIndex GetChildIndexWithID(const GameObjectID& gameObjectID) const;
		GameObjectID GetIDAtChildIndex(const ChildIndex& childIndex) const;

		// Filled if this object is a trigger
		std::vector<GameObject*> overlappingObjects;

		// Signals that connected objects get sent
		std::vector<i32> outputSignals;
		std::vector<Socket*> sockets;

		GameObjectID ID;

	protected:
		friend BaseScene;
		friend ResourceManager;

		static const char* s_DefaultNewGameObjectName;

		static AudioCue s_SqueakySounds;
		static AudioSourceID s_BunkSound;

		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs);
		virtual void ParseInstanceUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs);
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const;
		virtual void SerializeInstanceUniqueFields(JSONObject& parentObject) const;

		void CopyGenericFields(GameObject* newGameObject, GameObject* parent = nullptr, CopyFlags copyFlags = CopyFlags::ALL);

		void SetOutputSignal(i32 slotIdx, i32 value);

		bool GetChildIndexWithIDRecursive(const GameObjectID& gameObjectID, ChildIndex& outChildIndex) const;
		bool GetIDAtChildIndexRecursive(ChildIndex childIndex, GameObjectID& outGameObjectID) const;

		// Returns a string containing our name with a "_xx" post-fix where xx is the next highest index or 00

		std::string m_Name;

		std::vector<std::string> m_Tags;

		Transform m_Transform;

		StringID m_TypeID = InvalidStringID;

		// TODO: Store as bitfield

		/*
		* If true, this object will be written out to file
		* NOTE: If false, children will also not be serialized
		*/
		bool m_bSerializable = true;

		/*
		* Whether or not this object should be rendered
		* NOTE: Does *not* effect childrens' visibility
		*/
		bool m_bVisible = true;

		/*
		* Whether or not this object should be shown in the scene explorer UI
		* NOTE: Children are also hidden when this if false!
		*/
		bool m_bVisibleInSceneExplorer = true;

		/*
		* True if and only if this object will never move
		* If true, this object will be rendered to reflection probes
		*/
		bool m_bStatic = false;

		/*
		* If true this object will not collide with other game objects
		* Overlapping objects will cause OnOverlapBegin/End to be called
		*/
		bool m_bTrigger = false;

		/*
		* True if this object can currently be interacted with (can be based on
		* player proximity, among other things)
		*/
		bool m_bInteractable = false;

		bool m_bCastsShadow = true;

		// If true, this object will never live in the real world and will only be duplicated
		bool m_bIsTemplate = false;

		bool m_bSerializeMesh = true;

		// Editor only
		bool m_bUniformScale = false;

		PrefabID m_PrefabIDLoadedFrom = InvalidPrefabID;

		/*
		* Will point at the player we're interacting with, or the object if we're the player
		*/
		GameObject* m_ObjectInteractingWith = nullptr;

		GameObject* m_NearbyInteractable = nullptr;

		i32 m_SiblingIndex = 0;

		btCollisionShape* m_CollisionShape = nullptr;
		RigidBody* m_RigidBody = nullptr;

		GameObject* m_Parent = nullptr;
		std::vector<GameObject*> m_Children;

		Mesh* m_Mesh = nullptr;

	private:
		void DrawImGuiForSelfInternal();

	};

	// Child classes

	class DirectionalLight final : public GameObject
	{
	public:
		DirectionalLight();
		explicit DirectionalLight(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual void Initialize() override;
		virtual void Destroy(bool bDetachFromParent = true) override;
		virtual void Update() override;
		virtual void DrawImGuiObjects() override;
		virtual void SetVisible(bool bVisible, bool bEffectChildren /* = true */) override;
		virtual void OnTransformChanged() override;

		bool operator==(const DirectionalLight& other);

		void SetPos(const glm::vec3& newPos);
		glm::vec3 GetPos() const;
		void SetRot(const glm::quat& newRot);
		glm::quat GetRot() const;

		DirLightData data;

		// Editor-only
		glm::vec3 pos = VEC3_ZERO;

	protected:
		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;
	};

	class PointLight final : public GameObject
	{
	public:
		explicit PointLight(BaseScene* scene);
		explicit PointLight(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual GameObject* CopySelf(
			GameObject* parent = nullptr,
			CopyFlags copyFlags = CopyFlags::ALL,
			std::string* optionalName = nullptr,
			const GameObjectID& optionalGameObjectID = InvalidGameObjectID) override;

		virtual void Initialize() override;
		virtual void Destroy(bool bDetachFromParent = true) override;
		virtual void Update() override;
		virtual void DrawImGuiObjects() override;
		virtual void SetVisible(bool bVisible, bool bEffectChildren /* = true */) override;
		virtual void OnTransformChanged() override;

		bool operator==(const PointLight& other);

		void SetPos(const glm::vec3& pos);
		glm::vec3 GetPos() const;

		PointLightData data;
		PointLightID pointLightID = InvalidPointLightID;

	protected:
		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;
	};

	class Valve final : public GameObject
	{
	public:
		explicit Valve(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual GameObject* CopySelf(
			GameObject* parent = nullptr,
			CopyFlags copyFlags = CopyFlags::ALL,
			std::string* optionalName = nullptr,
			const GameObjectID& optionalGameObjectID = InvalidGameObjectID) override;

		virtual void PostInitialize() override;
		virtual void Update() override;

		// Serialized fields
		real minRotation = 0.0f;
		real maxRotation = 0.0f;

		// Non-serialized fields
		// Multiplied with value retrieved from input manager
		real rotationSpeedScale = 1.0f;

		// 1 = never slow down, 0 = slow down immediately
		real invSlowDownRate = 0.85f;

		real rotationSpeed = 0.0f;
		real pRotationSpeed = 0.0f;

		real rotation = 0.0f;
		real pRotation = 0.0f;

	protected:
		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

	};

	class RisingBlock final : public GameObject
	{
	public:
		explicit RisingBlock(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual GameObject* CopySelf(
			GameObject* parent = nullptr,
			CopyFlags copyFlags = CopyFlags::ALL,
			std::string* optionalName = nullptr,
			const GameObjectID& optionalGameObjectID = InvalidGameObjectID) override;

		virtual void Initialize() override;
		virtual void PostInitialize() override;
		virtual void Update() override;

		// Serialized fields
		Valve* valve = nullptr; // (object name is serialized)
		glm::vec3 moveAxis;

		// If true this block will "fall" to its minimum
		// value when a player is not interacting with it
		bool bAffectedByGravity = false;

		// Non-serialized fields
		glm::vec3 startingPos;

		real pdDistBlockMoved = 0.0f;

	protected:
		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

	};

	class GlassPane final : public GameObject
	{
	public:
		explicit GlassPane(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual GameObject* CopySelf(
			GameObject* parent = nullptr,
			CopyFlags copyFlags = CopyFlags::ALL,
			std::string* optionalName = nullptr,
			const GameObjectID& optionalGameObjectID = InvalidGameObjectID) override;

		bool bBroken = false;

	protected:
		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

	};

	class ReflectionProbe final : public GameObject
	{
	public:
		explicit ReflectionProbe(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual GameObject* CopySelf(
			GameObject* parent = nullptr,
			CopyFlags copyFlags = CopyFlags::ALL,
			std::string* optionalName = nullptr,
			const GameObjectID& optionalGameObjectID = InvalidGameObjectID) override;

		virtual void PostInitialize() override;

		MaterialID captureMatID = 0;

	protected:
		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

	};

	class Skybox final : public GameObject
	{
	public:
		explicit Skybox(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		void ProcedurallyInitialize(MaterialID matID);

	protected:
		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

		void InternalInit(MaterialID matID);
	};

	class EngineCart;

	class Cart : public GameObject
	{
	public:
		Cart(CartID cartID);
		Cart(CartID cartID,
			const std::string& name,
			const GameObjectID& gameObjectID = InvalidGameObjectID,
			StringID typeID = SID("cart"),
			const char* meshName = emptyCartMeshName);

		virtual GameObject* CopySelf(
			GameObject* parent = nullptr,
			CopyFlags copyFlags = CopyFlags::ALL,
			std::string* optionalName = nullptr,
			const GameObjectID& optionalGameObjectID = InvalidGameObjectID) override;

		virtual void DrawImGuiObjects() override;
		virtual real GetDrivePower() const;

		void OnTrackMount(TrackID trackID, real newDistAlongTrack);
		void OnTrackDismount();

		void SetItemHolding(GameObject* obj);
		void RemoveItemHolding();

		// Advances along track, rotates to face correct direction
		void AdvanceAlongTrack(real dT);

		// Returns velocity
		real UpdatePosition();

		CartID cartID = InvalidCartID;

		TrackID currentTrackID = InvalidTrackID;
		real distAlongTrack = -1.0f;
		real velocityT = 1.0f;

		real distToRearNeighbor = -1.0f;

		// Non-serialized fields
		real attachThreshold = 1.5f;

		Spring<real> m_TSpringToCartAhead;

		CartChainID chainID = InvalidCartChainID;

		static const char* emptyCartMeshName;

	protected:
		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

	};

	class EngineCart : public Cart
	{
	public:
		explicit EngineCart(CartID cartID);
		EngineCart(CartID cartID, const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual GameObject* CopySelf(
			GameObject* parent = nullptr,
			CopyFlags copyFlags = CopyFlags::ALL,
			std::string* optionalName = nullptr,
			const GameObjectID& optionalGameObjectID = InvalidGameObjectID) override;

		virtual void Update() override;
		virtual void DrawImGuiObjects() override;
		virtual real GetDrivePower() const override;


		real moveDirection = 1.0f; // -1.0f or 1.0f
		real powerRemaining = 1.0f;

		real powerDrainMultiplier = 0.1f;
		real speed = 0.1f;

		static const char* engineMeshName;

	protected:
		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

	};

	class MobileLiquidBox final : public GameObject
	{
	public:
		MobileLiquidBox();
		explicit MobileLiquidBox(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual GameObject* CopySelf(
			GameObject* parent = nullptr,
			CopyFlags copyFlags = CopyFlags::ALL,
			std::string* optionalName = nullptr,
			const GameObjectID& optionalGameObjectID = InvalidGameObjectID) override;

		virtual void DrawImGuiObjects() override;

		bool bInCart = false;
		real liquidAmount = 0.0f;

	protected:
		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

	};

	class GerstnerWave final : public GameObject
	{
	public:
		explicit GerstnerWave(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual void Initialize() override;
		virtual void Update() override;
		virtual void Destroy(bool bDetachFromParent = true) override;
		void AddWave();
		void RemoveWave(i32 index);

		virtual void DrawImGuiObjects() override;

		struct WaveInfo
		{
			bool enabled = true;
			real a = 0.0f;
			real waveDirTheta = 0.0f;
			real waveLen = 1.0f;

			// Non-serialized, calculated from fields above
			real waveDirCos;
			real waveDirSin;
			real moveSpeed = -1.0f;
			real waveVecMag = -1.0f;
			real accumOffset = 0.0f;
		};

		struct WaveTessellationLOD
		{
			WaveTessellationLOD(real squareDist, u32 vertCountPerAxis) :
				squareDist(squareDist),
				vertCountPerAxis(vertCountPerAxis)
			{}

			real squareDist;
			u32 vertCountPerAxis;
		};

		struct WaveSamplingLOD
		{
			WaveSamplingLOD(real squareDist, real amplitudeCutoff) :
				squareDist(squareDist),
				amplitudeCutoff(amplitudeCutoff)
			{}

			real squareDist;
			real amplitudeCutoff;
		};

		struct WaveChunk
		{
			WaveChunk(const glm::vec2i& index, u32 vertOffset, u32 tessellationLODLevel) :
				index(index),
				vertOffset(vertOffset),
				tessellationLODLevel(tessellationLODLevel)
			{}

			glm::vec2i index;
			u32 vertOffset;
			u32 tessellationLODLevel;
		};

		struct WaveGenData
		{
			// Inputs
			// General
			std::vector<WaveInfo> const* waveContributions;
			std::vector<WaveChunk> const* waveChunks;
			std::vector<WaveSamplingLOD> const* waveSamplingLODs;
			std::vector<WaveTessellationLOD> const* waveTessellationLODs;
			WaveInfo const* soloWave;
			real size;
			u32 chunkIdx;
			bool bDisableLODs;
			// Chunk-specific
			glm::vec3* positions;
			real blendDist;

			// Intermediate values:
			__m128* lodCutoffsAmplitudes_4 = nullptr;
			__m128* lodNextCutoffAmplitudes_4 = nullptr;
			__m128* lodBlendWeights_4 = nullptr;

			// Outputs:
			__m128* positionsx_4 = nullptr;
			__m128* positionsy_4 = nullptr;
			__m128* positionsz_4 = nullptr;
			__m128* lodSelected_4 = nullptr;
			__m128* uvUs_4 = nullptr;
			__m128* uvVs_4 = nullptr;
		};

		using ThreadID = u32;

		//struct Thread
		//{
		//	std::thread thread;
		//	bool bInUse;
		//};

		//struct ThreadData
		//{
		//	WaveGenData waveGenInOut;
		//	ThreadID threadID;
		//};

	private:
		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

		void UpdateDependentVariables(i32 waveIndex);

		void AllocWorkQueueEntry(u32 workQueueIndex);
		void FreeWorkQueueEntry(u32 workQueueIndex);

		void DiscoverChunks();
		void UpdateWaveVertexData();

		void UpdateWavesLinear();
		void UpdateWavesSIMD();
		glm::vec4 ChooseColourFromLOD(real LOD);
		glm::vec3 QueryHeightFieldFromVerts(const glm::vec3& queryPos) const;
		WaveChunk const* GetChunkAtPos(const glm::vec2& pos) const;
		WaveTessellationLOD const* GetTessellationLOD(u32 lodLevel) const;
		u32 ComputeTesellationLODLevel(const glm::vec2i& chunkIdx);
		void UpdateNormalsForChunk(u32 chunkIdx);
		void SortWaves();
		void SortWaveSamplingLODs();
		void SortWaveTessellationLODs();
		real GetWaveAmplitudeLODCutoffForDistance(real dist) const;

		real size = 30.0f;
		real loadRadius = 35.0f;
		real updateSpeed = 20.0f;
		real blendDist = 10.0f;
		bool bDisableLODs = false;
		u32 maxChunkVertCountPerAxis = 64;

		OceanData oceanData;

		void* criticalSection = nullptr;

		MaterialID m_WaveMaterialID;

		std::vector<WaveTessellationLOD> waveTessellationLODs;
		std::vector<WaveSamplingLOD> waveSamplingLODs;

		std::vector<WaveInfo> waveContributions;
		WaveInfo const* soloWave = nullptr;

		std::vector<WaveChunk> waveChunks;

		bool m_bPinCenter = false;
		glm::vec3 m_PinnedPos;

		VertexBufferDataCreateInfo m_VertexBufferCreateInfo;
		std::vector<u32> m_Indices;

		GameObject* bobber = nullptr;
		Spring<real> bobberTarget;

		RollingAverage<ms> avgWaveUpdateTime;

		u32 DEBUG_lastUsedVertCount = 0;

		ThreadData threadUserData;
	};

	bool operator==(const GerstnerWave::WaveInfo& lhs, const GerstnerWave::WaveInfo& rhs);

	void* ThreadUpdate(void* inData);

	GerstnerWave::WaveChunk const* GetChunkAtPos(const glm::vec2& pos, const std::vector<GerstnerWave::WaveChunk>& waveChunks, real size);
	GerstnerWave::WaveTessellationLOD const* GetTessellationLOD(u32 lodLevel, const std::vector<GerstnerWave::WaveTessellationLOD>& waveTessellationLODs);
	u32 MapVertIndexAcrossLODs(u32 vertIndex, GerstnerWave::WaveTessellationLOD const* lod0, GerstnerWave::WaveTessellationLOD const* lod1);

	class Blocks final : public GameObject
	{
	public:
		explicit Blocks(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual void Update() override;

	protected:

	};

	// Connects terminals to other things to transmit information
	class Wire final : public GameObject
	{
	public:
		Wire(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual void Destroy(bool bDetachFromParent = true) override;

		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

		void PlugIn(Socket* socket);
		void Unplug(Socket* socket);

		virtual bool AllowInteractionWith(GameObject* gameObject) override;
		virtual void SetInteractingWith(GameObject* gameObject) override;

		GameObjectID socket0ID = InvalidGameObjectID;
		GameObjectID socket1ID = InvalidGameObjectID;

		glm::vec3 startPoint;
		glm::vec3 endPoint;
	};

	// Connect wires to objects
	class Socket final : public GameObject
	{
	public:
		Socket(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual void Destroy(bool bDetachFromParent = true) override;

		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

		virtual bool AllowInteractionWith(GameObject* gameObject) override;
		virtual void SetInteractingWith(GameObject* gameObject) override;

		GameObject* parent = nullptr;
		Wire* connectedWire = nullptr;
		i32 slotIdx = 0;
	};

	// TODO: Add scene base class
	class PluggablesSystem
	{
	public:
		void Initialize();
		void Destroy();

		void Update();

		i32 GetReceivedSignal(Socket* socket);

		Wire* AddWire(const GameObjectID& gameObjectID, Socket* socket0 = nullptr, Socket* socket1 = nullptr);
		bool DestroySocket(Socket* socket);
		bool DestroyWire(Wire* wire);

		Socket* AddSocket(const std::string& name, const GameObjectID& gameObjectID, i32 slotIdx = 0, Wire* connectedWire = nullptr);
		Socket* AddSocket(Socket* socket, i32 slotIdx = 0, Wire* connectedWire = nullptr);

		std::vector<Wire*> wires;
		std::vector<Socket*> sockets;

	private:
		bool RemoveSocket(const GameObjectID& socketID);

		// TODO: Serialization (requires ObjectIDs)
		// TODO: Use WirePool

	};

	class Terminal final : public GameObject
	{
	public:
		Terminal();
		explicit Terminal(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual void Initialize() override;
		virtual void PostInitialize() override;
		virtual void Destroy(bool bDetachFromParent = true) override;
		virtual void Update() override;

		virtual bool AllowInteractionWith(GameObject* gameObject) override;

		void SetCamera(TerminalCamera* camera);

		void DrawTerminalUI();

		static const i32 MAX_OUTPUT_COUNT = 4;

		std::vector<Wire*> wireSlots;

	protected:
		void TypeChar(char c);
		void DeleteChar(bool bDeleteUpToNextBreak = false); // (backspace)
		void DeleteCharInFront(bool bDeleteUpToNextBreak = false); // (delete)
		void Clear();

		void MoveCursorToStart();
		void MoveCursorToStartOfLine();
		void MoveCursorToEnd();
		void MoveCursorToEndOfLine();
		void MoveCursorLeft(bool bSkipToNextBreak = false);
		void MoveCursorRight(bool bSkipToNextBreak = false);
		void MoveCursorUp();
		void MoveCursorDown();

		void ClampCursorX();

		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

	private:
		friend TerminalCamera;

		EventReply OnKeyEvent(KeyCode keyCode, KeyAction action, i32 modifiers);
		KeyEventCallback<Terminal> m_KeyEventCallback;

		bool SkipOverChar(char c);
		i32 GetIdxOfNextBreak(i32 y, i32 startX);
		i32 GetIdxOfPrevBreak(i32 y, i32 startX);

		void ParseCode();
		void EvaluateCode();

		VM::VirtualMachine* m_VM = nullptr;

		std::vector<std::string> lines;

		real m_LineHeight = 1.0f;
		real m_LetterScale = 0.04f;

		glm::vec2i cursor;
		// Keeps track of the cursor x to be able to position the cursor correctly
		// after moving from a long line, over a short line, onto a longer line again
		i32 cursorMaxX = 0;

		// Non-serialized fields:
		TerminalCamera* m_Camera = nullptr;
		const i32 m_CharsWide = 45;

		const sec m_CursorBlinkRate = 0.6f;
		sec m_CursorBlinkTimer = 0.0f;

	};

	class ParticleSystem final : public GameObject
	{
	public:
		explicit ParticleSystem(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual GameObject* CopySelf(
			GameObject* parent = nullptr,
			CopyFlags copyFlags = CopyFlags::ALL,
			std::string* optionalName = nullptr,
			const GameObjectID& optionalGameObjectID = InvalidGameObjectID) override;

		virtual void Destroy(bool bDetachFromParent = true) override;

		virtual void DrawImGuiObjects() override;

		virtual void OnTransformChanged() override;

		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

		glm::mat4 model;
		real scale;
		ParticleSimData data;
		bool bEnabled;
		MaterialID simMaterialID = InvalidMaterialID;
		MaterialID renderingMaterialID = InvalidMaterialID;
		ParticleSystemID particleSystemID = InvalidParticleSystemID;

	private:
		void UpdateModelMatrix();

	};

	class TerrainGenerator final : public GameObject
	{
	public:
		explicit TerrainGenerator(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual void Initialize() override;
		virtual void PostInitialize() override;
		virtual void Update() override;
		virtual void Destroy(bool bDetachFromParent = true) override;

		virtual void DrawImGuiObjects() override;

		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

		u32 VertCountPerChunkAxis = 8;
		real ChunkSize = 16.0f;
		real MaxHeight = 3.0f;

	private:
		void GenerateGradients();
		void GenerateChunk(const glm::ivec2& index);
		void DestroyAllChunks();
		real SampleTerrain(const glm::vec2& pos);
		real SampleNoise(const glm::vec2& pos, real octave, u32 octaveIdx);

		MaterialID m_TerrainMatID = InvalidMaterialID;
		std::map<glm::vec2i, MeshComponent*, Vec2iCompare> m_Meshes; // Chunk index to mesh

		real nscale = 1.0f;
		real m_LoadedChunkRadius = 100.0f;

		std::set<glm::vec2i, Vec2iCompare> m_ChunksToLoad;
		std::set<glm::vec2i, Vec2iCompare> m_ChunksToDestroy;

		const ns m_CreationBudgetPerFrame = Time::ConvertFormatsConstexpr(1.0f, Time::Format::MILLISECOND, Time::Format::NANOSECOND);
		const ns m_DeletionBudgetPerFrame = Time::ConvertFormatsConstexpr(0.5f, Time::Format::MILLISECOND, Time::Format::NANOSECOND);

		bool m_UseManualSeed = true;
		i32 m_ManualSeed = 0;

		real m_OctaveScale = 1.0f;
		real m_BaseOctave = 1.0f;
		u32 m_NumOctaves = 1;

		bool m_bHighlightGrid = false;
		bool m_bDisplayTables = false;

		bool m_bPinCenter = false;
		glm::vec3 m_PinnedPos;

		glm::vec3 m_LowCol;
		glm::vec3 m_MidCol;
		glm::vec3 m_HighCol;

		std::vector<std::vector<glm::vec2>> m_RandomTables;
		u32 m_BasePerlinTableWidth = 16;

		std::vector<TextureID> m_TableTextureIDs;

		i32 m_IsolateOctave = -1;

	};

	class SpringObject final : public GameObject
	{
	public:
		SpringObject(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual GameObject* CopySelf(
			GameObject* parent = nullptr,
			CopyFlags copyFlags = CopyFlags::ALL,
			std::string* optionalName = nullptr,
			const GameObjectID& optionalGameObjectID = InvalidGameObjectID) override;

		virtual void Initialize() override;
		virtual void Update() override;
		virtual void Destroy(bool bDetachFromParent = true) override;

		virtual void DrawImGuiObjects() override;

		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

		virtual void ParseJSON(
			const JSONObject& obj,
			BaseScene* scene,
			i32 fileVersion,
			MaterialID overriddenMatID = InvalidMaterialID,
			bool bIsPrefabTemplate = false,
			CopyFlags copyFlags = CopyFlags::ALL) override;

	private:
		static void CreateMaterials();

		static const char* s_ExtendedMeshFilePath;
		static const char* s_ContractedMeshFilePath;

		static MaterialID s_SpringMatID;
		static MaterialID s_BobberMatID;

		MeshComponent* m_ExtendedMesh = nullptr;
		MeshComponent* m_ContractedMesh = nullptr;

		VertexBufferDataCreateInfo m_DynamicVertexBufferCreateInfo;
		std::vector<u32> m_Indices;

		GameObject* m_Target = nullptr;
		real m_MinLength = 5.0f;
		real m_MaxLength = 10.0f;
		glm::vec3 m_TargetPos = VEC3_ZERO;

		GameObject* m_OriginTransform = nullptr;

		// We manage these objects ourselves rather than adding them to the scene
		bool m_bSimulateTarget = true;
		SoftBody* m_SpringSim = nullptr;
		GameObject* m_Bobber = nullptr;

		std::vector<glm::vec3> extendedPositions;
		std::vector<glm::vec3> extendedNormals;
		std::vector<glm::vec3> extendedTangents;
		std::vector<glm::vec3> contractedPositions;
		std::vector<glm::vec3> contractedNormals;
		std::vector<glm::vec3> contractedTangents;
	};

	struct Point
	{
		Point(glm::vec3 pos, glm::vec3 vel, real invMass) :
			pos(pos),
			vel(vel),
			invMass(invMass)
		{}

		glm::vec3 pos;
		glm::vec3 vel;
		real invMass;
	};

	struct Constraint
	{
		enum class EqualityType
		{
			EQUALITY,
			INEQUALITY,

			_NONE
		};

		enum class Type
		{
			DISTANCE,
			BENDING,

			_NONE
		};

		Constraint(real stiffness, EqualityType equalityType, Type type) :
			stiffness(stiffness),
			equalityType(equalityType),
			type(type)
		{
		}

		real stiffness;
		EqualityType equalityType;
		Type type;
	};

	struct DistanceConstraint : public Constraint
	{
		DistanceConstraint(i32 pointIndex0, i32 pointIndex1, real stiffness, real targetDistance);

		real targetDistance;
		i32 pointIndices[2];
	};

	struct BendingConstraint : public Constraint
	{
		BendingConstraint(i32 pointIndex0, i32 pointIndex1, i32 pointIndex2, i32 pointIndex3, real stiffness, real targetPhi);

		real targetPhi;
		i32 pointIndices[4];
	};

	struct Triangle
	{
		Triangle();
		Triangle(i32 pointIndex0, i32 pointIndex1, i32 pointIndex2);

		i32 pointIndices[3];
	};

	class SoftBody final : public GameObject
	{
	public:
		SoftBody(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual GameObject* CopySelf(
			GameObject* parent = nullptr,
			CopyFlags copyFlags = CopyFlags::ALL,
			std::string* optionalName = nullptr,
			const GameObjectID& optionalGameObjectID = InvalidGameObjectID) override;

		virtual void Initialize() override;
		virtual void Destroy(bool bDetachFromParent = true) override;
		virtual void Update() override;

		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;

		virtual void DrawImGuiObjects() override;

		void SetStiffness(real stiffness);
		void SetDamping(real damping);

		// Add new constraint between index0 & index1 if one doesn't already exist.
		// Returns new constraint count
		u32 AddUniqueDistanceConstraint(i32 index0, i32 index1, u32 atIndex, real stiffness);
		u32 AddUniqueBendingConstraint(i32 index0, i32 index1, i32 index2, i32 index3, u32 atIndex, real stiffness);

		static ms FIXED_UPDATE_TIMESTEP;
		static u32 MAX_UPDATE_COUNT; // Max fixed update steps that can be taken in one frame

		std::vector<Point*> points;
		// TODO: Split constraint types into separate containers
		std::vector<Constraint*> constraints;
		std::vector<Triangle*> triangles;

	private:
		void Draw();


		void LoadFromMesh();

		// Outside vert is vert not on shared edge
		bool GetTriangleSharingEdge(const std::vector<u32>& indexData, i32 edgeIndex0, i32 edgeIndex1, const Triangle& originalTri, Triangle& outTri, i32& outOutsideVertIndex);

		u32 m_SolverIterationCount;
		bool m_bPaused = false;
		bool m_bSingleStep = false;
		bool m_bRenderWireframe = true;

		ms m_MSToSim = 0.0f;
		ms m_UpdateDuration = 0.0f;
		real m_Damping = 0.99f;
		real m_Stiffness = 0.8f;
		real m_BendingStiffness = 0.1f;

		u32 m_DragPointIndex = 0;

		i32 m_ShownBendingIndex = 0;
		i32 m_FirstBendingConstraintIndex = 0;

		std::vector<glm::vec3> initialPositions;

		Mesh* m_Mesh = nullptr;
		MeshComponent* m_MeshComponent = nullptr;
		VertexBufferDataCreateInfo m_MeshVertexBufferCreateInfo;
		MaterialID m_MeshMaterialID = InvalidMaterialID;

		std::string m_CurrentMeshFilePath;
		// Editor only
		i32 m_SelectedMeshIndex = -1;
		std::string m_CurrentMeshFileName;

	};

	static const char* TireNames[] = { "FL", "FR", "RL", "RR", "None" };

	class Vehicle final : public GameObject
	{
	public:
		Vehicle(const std::string& name, const GameObjectID& gameObjectID = InvalidGameObjectID);

		virtual GameObject* CopySelf(
			GameObject* parent = nullptr,
			CopyFlags copyFlags = CopyFlags::ALL,
			std::string* optionalName = nullptr,
			const GameObjectID& optionalGameObjectID = InvalidGameObjectID) override;

		virtual void Initialize() override;
		virtual void Destroy(bool bDetachFromParent = true) override;
		virtual void Update() override;

		virtual void ParseTypeUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void ParseInstanceUniqueFields(const JSONObject& parentObject, BaseScene* scene, const std::vector<MaterialID>& matIDs) override;
		virtual void SerializeTypeUniqueFields(JSONObject& parentObject) const override;
		virtual void SerializeInstanceUniqueFields(JSONObject& parentObject) const override;

		virtual void DrawImGuiObjects() override;

	private:
		enum class Tire : u32
		{
			FL = 0,
			FR = 1,
			RL = 2,
			RR = 3,

			_NONE
		};

		static_assert((ARRAY_LENGTH(TireNames) - 1) == (i32)Tire::_NONE, "TireNames length does not match number of entires in Tire enum");

		static const i32 m_TireCount = 4;

		GameObjectID m_TireIDs[m_TireCount];

		real m_EngineForce = 0.0f;
		real m_BrakeForce = 0.0f;
		real m_Steering = 0.0f;
		const real MAX_STEER = 0.5f;
		const real MAX_ENGINE_FORCE = 2500.0f;
		const real ENGINE_FORCE_SLOW_FACTOR = 0.5f;

#if 0
		class CommonExampleInterface* vehicle;
#endif

		btAlignedObjectArray<btCollisionShape*> m_collisionShapes;

		btDefaultVehicleRaycaster* m_VehicleRaycaster;
		btRaycastVehicle* m_Vehicle;

		btRaycastVehicle::btVehicleTuning m_tuning;

	};

} // namespace flex
