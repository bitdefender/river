# Tensorflow 2 implementation
import numpy as np
import tensorflow as tf
import os
import sys
import RiverUtils
from typing import List, Dict


# TODO:
# 1. Optimize parameters + add option to stop training when no improvement
# 3. optimize experience replay method: Similar experiences such as (state, action, value) should be detected and eliminated ! ->  !

Batch_size = 64
BBlock_emb_size = 10
action_emb_size = 30
numEpochs = 100
maxEmbeddingPathStateSize = 50
maskValueForRNNState = 0

num_exp_to_triggerRetrain = Batch_size * 32
NEGATIVE_ACTION_SCORE = -10  # Invalid path action, e.g. no continuation from here

class RLBanditsModule():
	def __init__(self):
		self.model = self.buildRLConcolicModel(batchSize=Batch_size, BBlock_emb_size=BBlock_emb_size,action_emb_size=action_emb_size)

		# The data gathered as experiences  TODO: could improve a lot this...
		self.currentExperiences_states = []
		self.currentExperiences_actions = []
		self.currentExperiences_realValues = []

	# Given a list of basic blocks this func creates an embedding space of the requested size out of it
	# TODO: include a list of visited bblocks as well here ?
	def buildRLGenerationalStateEmbeeding(self, blocksPath: List[int], embeddingMaxSize) -> List[int]:
		# Cut if too long
		if len(blocksPath) > embeddingMaxSize:
			blocksPath = blocksPath[-embeddingMaxSize:]

		# Make the path relative to the first input
		if len(blocksPath) > 0:
			offsetBegin = blocksPath[0] - 1
			blocksPath[0] -= offsetBegin
			for i in range(1, len(blocksPath)):
				blocksPath[i] -= offsetBegin

		# Append if too short
		"""
		diffLen = embeddingMaxSize - len(blocksPath)
		if diffLen > 0:
			blocksPath.extend([maskValueForRNNState] * diffLen)  # THe difference can't be 0 in the normal because we made a jump between two consecutive blocks and this helps us a lot
		"""

		#assert len(blocksPath) == embeddingMaxSize
		return blocksPath #tf.constant(blocksPath)

	def addExperience(self, bb_path_state, action, realValue): # realValue is the concrete reward value that should be for (state,action)
		self.currentExperiences_actions.append(self.buildRLGenerationalStateEmbeeding(bb_path_state, maxEmbeddingPathStateSize))
		self.currentExperiences_states.append(action)
		self.currentExperiences_realValues.append(realValue)

		if len(self.currentExperiences_realValues) >= num_exp_to_triggerRetrain:
			# train the model with a new online batch of data
			self.model.train(dataSet_X_state=tf.constant(self.currentExperiences_actions),
							 dataSet_X_actions=tf.constant(self.currentExperiences_states),
							dataSet_Y=tf.constant(self.currentExperiences_realValues), epochs=numEpochs)

			# Clear experiences gathered
			self.currentExperiences_actions.clear()
			self.currentExperiences_states.clear()
			self.currentExperiences_realValues.clear()


	# Predict the value of a input and state
	def predict(self, input: RiverUtils.InputRLGenerational, basicBlocksPathFoundThisRun) -> float:
		bb_path_state = self.buildRLGenerationalStateEmbeeding(basicBlocksPathFoundThisRun, maxEmbeddingPathStateSize)
		bb_path_state = tf.expand_dims(bb_path_state, axis=0)
		#bb_path_state = tf.keras.preprocessing.sequence.pad_sequences(bb_path_state, padding="post")
		action_state = tf.constant([input.action])

		res = self.model(input_state=bb_path_state, input_action=action_state, training=False)
		return res.numpy()[0][0]

	# Given the size of a single basic block embedding size and
	# action embedding size (which in our case it is the maximum of the sequence length of considered bblocks path)
	# Return the model
	def buildRLConcolicModel(self, batchSize, BBlock_emb_size, action_emb_size):
		rnn_units = 100
		dense1_units = (BBlock_emb_size + action_emb_size)
		dense2_units = action_emb_size

		class RLConcolicModel(tf.keras.Model):
			def __init__(self,
						 batchSize, BBlock_emb_size, action_emb_size,  # meta parameters
						 rnn_units, dense1_units, dense2_units):  # Model config params
				super(RLConcolicModel, self).__init__()
				self.batchSize = batchSize
				# inputLayer_state = tf.keras.layers.Input(batch_shape=(B_size, Emb_size, 1), name='state')
				# inputLayer_action = tf.keras.layers.Input(batch_shape=(B_size, Emb_size), name='action')

				self.bblocksEncodingLayer = tf.keras.layers.Dense(BBlock_emb_size)

				# Process the state with a LSTM layer
				self.gruLayer = tf.keras.layers.LSTM(rnn_units)
				self.gruLayerReshaped = tf.keras.layers.Reshape(target_shape=(-1,))
				self.denseLayer1 = tf.keras.layers.Dense(dense1_units, activation='relu')
				self.denseLayer1_andAction = tf.keras.layers.Concatenate(axis=-1)
				self.denseLayer2 = tf.keras.layers.Dense(dense2_units, activation='relu')
				self.outputLayer = tf.keras.layers.Dense(1)

				self.BBlock_emb_size = BBlock_emb_size
				self.action_emb_size = action_emb_size

				# model = tf.keras.Model(inputs=[inputLayer_state, inputLayer_action], outputs=[outputVal])

				self.optimizer = tf.keras.optimizers.Adam(learning_rate=1e-3)
				self.lossFunc = tf.keras.losses.MeanSquaredError()

			def __call__(self, input_state, input_action, training):
				model.training = training

				# Embedding for bblocks path
				input_state = tf.expand_dims(input_state, axis=-1)
				input_state = self.bblocksEncodingLayer(input_state)
				#input_state = tf.keras.preprocessing.sequence.pad_sequences(input_state, padding="post")

				# Transforms actions to one hot encoding
				input_action = tf.one_hot(indices=input_action, depth=self.action_emb_size, dtype=tf.float32)

				input_state = self.gruLayer(input_state)
				input_state = self.gruLayerReshaped(input_state)

				input_state = self.denseLayer1(input_state)
				stateEmb_and_Action = self.denseLayer1_andAction(([input_state, input_action]))
				stateEmb_and_Action = self.denseLayer2(stateEmb_and_Action)

				output = self.outputLayer(stateEmb_and_Action)
				return output

			def train(self, dataSet_X_state, dataSet_X_actions, dataSet_Y, epochs):
				print("Starting training mode...")
				self.training = True
				assert dataSet_X_state.shape[0] == dataSet_Y.shape[0] == dataSet_X_actions.shape[0], "inp/out given doesn't match on batch size"
				itersPerEpoch = dataSet_X_state.shape[0] // self.batchSize

				# Iterate over epochs.
				for epoch in range(epochs):
					loss_metric = tf.keras.metrics.Mean()
					loss_metric.reset_states()

					print("Start of epoch %d" % (epoch,))

					# Iterate over the batches of the dataset.
					for step in range(itersPerEpoch):
						startBatchPos = step * self.batchSize
						endBatchPos = (step + 1) * self.batchSize
						xstate_batch = dataSet_X_state[startBatchPos: endBatchPos]
						xactions_batch = dataSet_X_actions[startBatchPos: endBatchPos]
						y_batch = dataSet_Y[startBatchPos: endBatchPos]

						with tf.GradientTape() as tape:
							y_pred_batch = self(input_state=xstate_batch,
												input_action=xactions_batch,
												training=True)

							# Compute loss
							loss = self.lossFunc(y_batch, y_pred_batch)

						loss_metric.update_state(loss)

						grads = tape.gradient(loss, self.trainable_weights)
						self.optimizer.apply_gradients(zip(grads, self.trainable_weights))

						if step % 100 == 0:
							print(f"Epoch:{epoch} step:{step}: mean loss = {loss_metric.result():.4f}")

				print(f"Epoch:{epoch} ended step:{step}: mean loss = {loss_metric.result():.4f}")

		model = RLConcolicModel(batchSize=batchSize,
								BBlock_emb_size=BBlock_emb_size,
								action_emb_size=action_emb_size,
								rnn_units=rnn_units,
								dense1_units=dense1_units,
								dense2_units=dense2_units)
		return model


if __name__ == "__main__":
	rlBandits = RLBanditsModule()

	def randomExp():
		state = list(np.random.choice(10, size=np.random.randint(10)))
		action = np.random.randint(10)
		realValue = np.random.rand()
		return state, action, realValue

	# Create random experiences to trigger a training
	for expIndex in range(Batch_size):
		rlBandits.addExperience(*randomExp())

	# Predict for some values
	state, action, realValue = randomExp()
	input = RiverUtils.InputRLGenerational()
	input.action = action
	print(rlBandits.predict(input, state))

	"""
	input_state = tf.random.uniform(shape=(Batch_size, 11))  # 11 is variable, can be anything !
	input_action = tf.random.uniform(shape=(Batch_size,), minval=0, maxval=action_emb_size, dtype=tf.int32)
	output_value = tf.random.uniform(shape=(Batch_size,), dtype=tf.float32)

	model = buildRLConcolicModel(Batch_size, BBlock_emb_size, action_emb_size)

	model.train(dataSet_X_state=input_state, dataSet_X_actions=input_action,
				dataSet_Y=output_value, epochs=numEpochs)

	outputs = model(input_state, input_action, training=False)  # model(input_state, input_action) #m({'state':input_state, 'action':input_action})
	"""